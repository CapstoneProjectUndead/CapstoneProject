#include "stdafx.h"
#include "GameScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"
#include "SceneManager.h"

// 맵 생성 알고리즘
#include "MapGenerator/MapGenerator.h"

CGameScene::CGameScene()
	: CScene(SCENE_TYPE::GAME)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	// 렌더링할 때 필요한 쉐이더 객체 생성
	if (shaders.empty()) {
		{
			// static shader
			std::shared_ptr<CShader> shader = std::make_unique<CShader>();
			shader->CreateShader(GET_DEVICE);
			shaders.emplace("static", std::move(shader));
		}
		{
			// skinning
			std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
			shader->CreateShader(GET_DEVICE);
			shaders.emplace("skinning", std::move(shader));
		}
	}

	if (prototypes.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["static"]->GetHeapManager() };
		prototypes = factory->CreateGameScene(staticHeapManager);
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
	}
	else {
		factory->SetComponent(dynamic_pointer_cast<CPlayer>(my_player));
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

void CGameScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
	CPhysicsManager::GetInstance().Update(elapsedTime);

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
	}
}

void CGameScene::DrawUI()
{
}

bool CGameScene::IsUIInputEnabled()
{
	bool state = true;

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene);

	if (scene->GetSceneType() == SCENE_TYPE::GAME)
		state = false;

	return state;
}

void CGameScene::Enter()
{
	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::LOBBY);
		camera->SetTarget(my_player.get());
	}
}

void CGameScene::Exit()
{
}