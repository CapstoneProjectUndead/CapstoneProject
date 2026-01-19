#include "stdafx.h"
#include "TestScene.h"
#include "Player.h"
#include "MyPlayer.h"

CTestScene::CTestScene()
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	//my_player = std::make_shared<CMyPlayer>(device, commandList);
	//camera = my_player->GetCameraPtr();

	// create graphics rootsignature
	graphics_root_signature = CreateGraphicsRootSignature(device);

	std::shared_ptr<CShader> shader = std::make_unique<CShader>();
	shader->CreateShader(device, graphics_root_signature.Get());
	//shader->BuildObjects(device, commandList);
	shaders.push_back(std::move(shader));

	// model load
	/*std::ifstream bin("../Modeling/undead_char.bin", std::ios::binary);
	std::ofstream txt("output.txt");

	char ch;
	while (bin.get(ch)) {
		txt << ch;   // txt 파일에 문자 그대로 출력
	}*/
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	
}

void CTestScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);

}
