#include "stdafx.h"
#include "TestScene.h"
#include "MyPlayer.h"
#include "Camera.h"

CTestScene::CTestScene()
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	//my_player = std::make_shared<CMyPlayer>();
	//my_player->Initialize(device, commandList);
	//// material set
	//Material m{};
	//m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	//m.glossiness = 0.0f;
	//my_player->SetMaterial(m);
	//
	//my_player->CreateConstantBuffers(device, commandList);
	//
	//std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	//shader->CreateShader(device);
	//shaders.push_back(std::move(shader));
	//
	//// test 용 삭제X
	//{
	//	auto obj = std::make_shared<CCharacter>();
	//	obj->Initialize(device, commandList);
	//	obj->SetMaterial(m);
	//
	//	obj->CreateConstantBuffers(device, commandList);
	//	objects.push_back(std::move(obj));
	//	
	//
	//	//std::ifstream bin("../Modeling/Undead_Lobby.bin", std::ios::binary);
	//	//std::ofstream txt("../Modeling/lobby.txt");
	//
	//	//char ch;
	//	//while (bin.get(ch)) {
	//	//	txt << ch;   // txt 파일에 문자 그대로 출력
	//	//}
	//}
	///*{
	//	auto obj = std::make_shared<CObject>();
	//	std::string filename{ "../Modeling/Undead_Lobby.bin" };
	//	auto frameRoot = CGeometryLoader::LoadGeometry(filename, device, commandList);
	//	obj->SetMesh(frameRoot->mesh);
	//
	//
	//	obj->CreateConstantBuffers(device, commandList);
	//	objects.push_back(std::move(obj));
	//}*/
	//
	//// 카메라 객체 생성
	//RECT client_rect;
	//GetClientRect(ghWnd, &client_rect);
	//float width{ float(client_rect.right - client_rect.left) };
	//float height{ float(client_rect.bottom - client_rect.top) };
	//
	//camera = std::make_shared<CCamera>();
	//camera->SetViewport(0, 0, width, height);
	//camera->SetScissorRect(0, 0, width, height);
	//camera->GenerateProjectionMatrix(1.0f, 500.0f, (float)width / (float)height, 90.0f);
	//camera->SetCameraOffset(XMFLOAT3(0.0f, 2.0f, -2.0f));
	//camera->SetTarget(my_player.get());
	//
	//camera->CreateConstantBuffers(device, commandList);
	//
	//// light 생성
	//light = std::make_unique<CLightManager>();
	//light->Initialize(device, commandList);
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);
}
