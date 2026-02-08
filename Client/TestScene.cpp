#include "stdafx.h"
#include "TestScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"
#include "Object.inl"
#include "Texture.h"

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
		// 수정 필요, 로드에 너무 오래 걸림
		// lobby_uv 로드
		std::string fileName{ "../Modeling/lobby_uv.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		CDescriptorHeapManager* heapManager{ shaders["static"]->GetHeapManager() };
		CMaterialManager matManager{};
		CTextureManager texManager{};
		// child value -> object
		for (const auto& children : frameRoot->childrens) {
			if (children->mesh.positions.empty()) break;
			auto obj = std::make_shared<CObject>();
			obj->SetMeshFromFile<CMatVertex>(device, commandList, children);
			// SetMaterial
			std::string name{ children->mesh.materials[0].albedoMap };	// material 하나 소유함
			auto tex = texManager.GetTexture(device, commandList, heapManager, name);
			auto mat = matManager.GetMeterial(name, tex);
			obj->SetMaterial(mat);

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
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);
}
