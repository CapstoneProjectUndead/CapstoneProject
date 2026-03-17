#include "stdafx.h"
#include "GameScene.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "ObjectFactory.h"
#include "SceneManager.h"
#include "ItemFinder.h"


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
	{
		// inst
		std::shared_ptr<CShader> shader = std::make_unique<CInstShader>();
		shader->CreateShader(GET_DEVICE);
		shaders.emplace("inst", std::move(shader));
	}

	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ shaders["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(staticHeapManager);
		treasures = factory->GetTreauseres();
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		my_player->SetPosition(0.0f, 2.0f, 0.0f);

		// 다우징 로드가 관리하는 treasuer_position(vector)에 보물 위치 정보를 넣는다.
		auto itemFinder = my_player->GetComponent<CItemFinder>();
		if (itemFinder) {
			itemFinder->RegisterTreasures(treasures);
		}
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

void CGameScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (const auto& shader : shaders) {
		shader.second->RenderBegin(commandList);

		if (camera)
			camera->UpdateShaderVariables(commandList);

		if (light)
			light->Render(commandList);

		for (const auto& obj : objects) {
			if (shader.first == obj->GetShader() && shader.first != "inst") {
				shader.second->Render(commandList, obj.get());
			}
		}

		if (shader.first == "inst") {
			shader.second->Render(commandList, nullptr);
		}

		if (my_player) {
			if (shader.first == my_player->GetShader()) {
				shader.second->Render(commandList, my_player.get());
			}
		}

		shader.second->RenderEnd(commandList);
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

		// 다우징 로드가 관리하는 treasuer_position(vector)에 보물 위치 정보를 넣는다.
		auto itemFinder = my_player->GetComponent<CItemFinder>();
		if (itemFinder) {
			itemFinder->RegisterTreasures(treasures);
		}
	}
}