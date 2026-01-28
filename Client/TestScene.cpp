#include "stdafx.h"
#include "TestScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Mesh.h"
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
	
	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(device);
	shaders.push_back(std::move(shader));

	// test 용 삭제X
	{
		auto obj = std::make_shared<CCharacter>();
		obj->Initialize(device, commandList);
		objects.push_back(std::move(obj));

		//std::ifstream bin("../Modeling/Undead_Lobby.bin", std::ios::binary);
		//std::ofstream txt("../Modeling/char.txt");

		//char ch;
		//while (bin.get(ch)) {
		//    if (
		//        ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ||   // 들여쓰기/띄어쓰기 유지
		//        (ch >= 'A' && ch <= 'Z') ||
		//        (ch >= 'a' && ch <= 'z') ||
		//        ch == '<' || ch == '>' || ch == '/'
		//       )
		//    {
		//        txt << ch;
		//    }
		//}
	}
	{

		// 파이 로드
		std::string fileName{ "../Modeling/Undead_Lobby.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);
		for (const auto& children : frameRoot->childrens) {
			auto obj = std::make_shared<CObject>();
			auto mesh = std::make_shared<CMesh>();
			if (children->mesh.positions.empty()) break;

			mesh->BuildVertices<CVertex>(device, commandList, children->mesh);
			mesh->SetIndices(device, commandList, (UINT)children->mesh.indices.size(), children->mesh.indices);
			obj->SetMesh(mesh);
			obj->world_matrix = children->localMatrix;

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
