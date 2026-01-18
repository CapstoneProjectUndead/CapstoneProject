#include "stdafx.h"
#include "TestScene.h"
#include "Player.h"

CTestScene::CTestScene()
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	//player = std::make_shared<CPlayer>(device, commandList);
	//camera = player->GetCameraPtr();

	// create graphics rootsignature
	graphics_root_signature = CreateGraphicsRootSignature(device);

	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(device, graphics_root_signature.Get());
	//shader->BuildObjects(device, commandList);
	shaders.push_back(std::move(shader));
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);

}
