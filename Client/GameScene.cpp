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
#include "NetworkManager.h"
#include "MeshRenderer.h"
#include "Inventory.h"


CGameScene::CGameScene()
	: CScene(SCENE_TYPE::GAME)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	if (objects.empty()) {
		CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(staticHeapManager);
		treasures = factory->GetTreauseres();
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };
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

	for (const auto& shader : CSceneManager::GetInstance().GetShaders()) {
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
	if (my_player) {
		auto& inventory = my_player->GetInventory();
		assert(inventory);
		inventory->Draw();
	}
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

void CGameScene::Exit()
{
	CScene::Exit();

	my_player = nullptr;
}

void CGameScene::Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt)
{
	const int cnt = pkt.data_count;	
	for (UINT i = 0; i < cnt; ++i) {
		instance_data.push_back(pkt.data[i]);
	}
}

void CGameScene::Handle_S_MapEnd(std::shared_ptr<Session> session, const S_MapEnd& pkt)
{
	// 기존 GameScene의 맵 데이터가 있다면 모두 clear
	objects.erase(std::remove_if(objects.begin(), objects.end(), [](const std::shared_ptr<CObject> obj) {
		return obj->GetObjectType() == OBJECT_TYPE::STATIC_OBJECT;
		}),
		objects.end());

	// CInstRenderer의 batches clear
	CInstRenderer::GetInstance().Clear();

	CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()["inst"]->GetHeapManager() };
	objects = factory->CreateGameSceneByServer(staticHeapManager, instance_data);
}
