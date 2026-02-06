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
		// Undead_Lobby 로드
		std::string fileName{ "../Modeling/Undead_Lobby.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);
		for (const auto& children : frameRoot->childrens) {
			if (children->mesh.positions.empty()) break;
			auto obj = std::make_shared<CObject>();
			obj->SetMeshFromFile<CVertex>(device, commandList, children);
			obj->Initialize(device, commandList);
			// texture
			std::shared_ptr<CTexture> tex = std::make_shared<CTexture>(std::string("floor"));
			tex->CreateTextureResource(device, commandList, std::wstring(L"../Modeling/tex/Rock.dds"));
			CDescriptorHeapManager* heapManager{ shaders["static"]->GetHeapManager() };
			UINT srvIndex = heapManager->Allocate();
			tex->SetDescriptorIndex(srvIndex);
			tex->CreateSrv(device, heapManager->GetCPUHandle(srvIndex));
			// material
			std::shared_ptr<CMaterial> m = std::make_shared<CMaterial>(std::string("floor"));
			m->SetTexture(tex);
			obj->SetMaterial(m);

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
