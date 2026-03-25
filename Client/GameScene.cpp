#include "stdafx.h"
#include "GameScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"
#include "SceneManager.h"
#include "MeshRenderer.h"
#include "Renderers.h"

#include "UIComponent.h"

CGameScene::CGameScene()
	: CScene(SCENE_TYPE::GAME)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	{
		// skinning
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("skinning", std::move(shader));
	}
	{
		// inst
		std::shared_ptr<CShader> shader = std::make_unique<CInstShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("inst", std::move(shader));
	}
	{
		std::shared_ptr<CShader> shader = std::make_unique<CBillboardShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("billboard", std::move(shader));
	}
	{
		// UI
		std::shared_ptr<CShader> shader = std::make_unique<CUIShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("ui", std::move(shader));
	}

	factory->GetMaterial(shaders["ui"]->GetHeapManager(), "white");	// 인덱스 0에 생성하기 위해 먼저 생성

	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(staticHeapManager);
	}

	{	// 나중에 sceneManager로 옮기기
		auto instRenderer = std::make_unique<CInstRenderer>();
		instRenderer->Initialize(GET_DEVICE, objects.size());
		renderers["inst"] = std::move(instRenderer);

		auto uiRenderer = std::make_unique<CUIRenderer>();
		uiRenderer->Initialize(GET_DEVICE, 100);
		renderers["ui"] = std::move(uiRenderer);

		auto bbRenderer = std::make_unique<CBillboardRenderer>();
		bbRenderer->Initialize(GET_DEVICE, 500);
		renderers["billboard"] = std::move(bbRenderer);

		// 20부터 font용(아직 제한X)
		CDescriptorHeapManager* heap = shaders["ui"]->GetHeapManager();
		auto textRenderer = std::make_unique<CTextRenderer>();
		textRenderer->Initialize(GET_DEVICE, GET_CMD_QUEUE, heap->GetCPUHandle(20), heap->GetGPUHandle(20));
		renderers["text"] = std::move(textRenderer);
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		my_player->SetPosition(0.0f, 2.0f, 0.0f);
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
	CScene::Enter();

	BuildObjects(GET_DEVICE, GET_CMD_LIST);

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::GAME);
		camera->SetTarget(my_player.get());
	}
}