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
	// model load
	frames["undead_char"] = CGeometryLoader::LoadGeometry("../Modeling/undead_char.bin", device, commandList);
	my_player = std::make_shared<CMyPlayer>();
	my_player->SetMesh(frames["undead_char"]->mesh);
	// material set
	Material m{};
	m.name = "Red";
	m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	m.roughness = 0.5f;
	m.metallic = 0.1f;
	my_player->SetMaterial(m);

	my_player->CreateConstantBuffers(device, commandList);

	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(device);
	shaders.push_back(std::move(shader));

	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	camera = std::make_shared<CCamera>();
	camera->SetViewport(0, 0, width, height);
	camera->SetScissorRect(0, 0, width, height);
	camera->GenerateProjectionMatrix(1.0f, 500.0f, (float)width / (float)height, 90.0f);
	camera->SetCameraOffset(XMFLOAT3(0.0f, 2.0f, -5.0f));
	camera->SetTarget(my_player.get());

	camera->CreateConstantBuffers(device, commandList);
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);

}
