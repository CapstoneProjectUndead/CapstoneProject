#include "stdafx.h"
#include "LobbyScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"

CLobbyScene::CLobbyScene()
	: CScene(SCENE_TYPE::LOBBY)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	{
		// static shader
		std::shared_ptr<CShader> shader = std::make_unique<CShader>();
		shader->CreateShader(device);
		shaders.emplace("static", std::move(shader));
	}
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(device);
		shaders.emplace("skinning", std::move(shader));
	}

	// factory
	CObjectFactory factory;
	
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory.CreateMyPlayer(skinningHeapManager);
	}
	
	{
		CDescriptorHeapManager* staticHeapManager{ shaders["static"]->GetHeapManager() };
		objects = factory.CreateLobby(staticHeapManager);
	}
	// test 용 삭제X
	{
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

	if (!camera) {
		camera = std::make_shared<CCamera>();
		camera->SetTarget(my_player.get());
		camera->Initialize(device, commandList);
	}
	
	// light 생성
	if (!light) {
		light = std::make_unique<CLightManager>();
		light->Initialize(device, commandList);
	}
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	CPhysicsManager::GetInstance().Update(elapsedTime);
}

void CLobbyScene::Render(ID3D12GraphicsCommandList* commandList)
{
	CScene::Render(commandList);
}

void CLobbyScene::Enter()
{
	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player)
		my_player->SetCurrentSceneType(SCENE_TYPE::LOBBY);
}

void CLobbyScene::Exit()
{
	my_player = nullptr;
	objects.clear();
	shaders.clear();
}