#include "stdafx.h"
#include "TestScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "MeshComponent.inl"
#include "Texture.h"
#include "Mesh.h"
#include "Collider.h"
#include "PhysicsManager.h"

CTestScene::CTestScene()
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	my_player = std::make_shared<CMyPlayer>();
	my_player->Initialize(device, commandList);
	
	{
		// static shader
		std::shared_ptr<CShader> shader = std::make_unique<CShader>();
		shader->CreateShader(device);
		shaders.emplace("static",std::move(shader));
	}
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(device);
		shaders.emplace("skinning", std::move(shader));
	}
	
	// test 용 삭제X
	{
		auto obj = std::make_shared<CCharacter>();
		obj->Initialize(device, commandList);
		objects.push_back(std::move(obj));
	
		/*std::ifstream bin("../Modeling/undead_char.bin", std::ios::binary);
		std::ofstream txt("../Modeling/char.txt");
	
		char ch;
		while (bin.get(ch)) {
		    if (
		        ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || (ch >= 'A' && ch <= 'Z') ||
		 (ch >= 'a' && ch <= 'z') || ch == '<' || ch == '>' || ch == '/' )
		    {
		        txt << ch;
		    }
		}*/
	}
	{
		std::string fileName{ "../Modeling/lobby_uv.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		CDescriptorHeapManager* heapManager{ shaders["static"]->GetHeapManager() };
		CMaterialManager matManager{};
		CTextureManager texManager{};

		for (const auto& children : frameRoot->childrens) {
			if (children->mesh.positions.empty()) break;
			auto obj = std::make_shared<CObject>();
			// 1) MeshComponent 생성
			auto meshComp = std::make_shared<CMeshComponent>();
			obj->SetComponent(meshComp);
			meshComp->SetMeshFromFile<CMatVertex>(device, commandList, children);
			obj->world_matrix = children->localMatrix;

			// 2) MaterialComponent 생성
			auto matComp = std::make_shared<CMaterialComponent>();
			obj->SetComponent(matComp);

			std::string name{ children->mesh.materials[0].albedoMap };
			auto tex = texManager.GetTexture(device, commandList, heapManager, name);
			auto mat = matManager.GetMeterial(name, tex);
			matComp->SetMaterial(mat);

			// 3) MeshRendererComponent 생성
			obj->SetComponent(std::make_shared<CMeshRendererComponent>());

			// 4) ColliderComponent 생성
			auto boxCollider = std::make_shared<CBoxColliderComponent>();
			obj->SetComponent(boxCollider);
			boxCollider->SetLocalBounds(children->mesh.bounds);
			//CPhysicsManager::GetInstance().SetCollider(boxCollider);

			obj->Initialize(device, commandList);

			objects.push_back(std::move(obj));
		}
	}
	
	camera = std::make_shared<CCamera>();
	camera->SetTarget(my_player.get());
	camera->Initialize(device, commandList);
	
	// light 생성
	light = std::make_unique<CLightManager>();
	light->Initialize(device, commandList);
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	CPhysicsManager::GetInstance().Update(elapsedTime);
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);
}
