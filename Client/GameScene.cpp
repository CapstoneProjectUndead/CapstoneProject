#include "stdafx.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "PhysicsManager.h"
#include "GameFramework.h"

#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "ObjectFactory.h"

#include "ItemFinder.h"
#include "NetworkManager.h"
#include "Inventory.h"
#include "WorldItem.h"
#include "ItemFactory.h"

#include "KeyManager.h"
#include "UIComponent.h"
#include "Movement.h"

CGameScene::CGameScene()
	: CScene(SCENE_TYPE::GAME)
{
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	auto shaders = CSceneManager::GetInstance().GetShaders();
	

	if (objects.empty()) {
		CDescriptorHeapManager* heapManager{ shaders["inst"]->GetHeapManager() };
		objects = factory->CreateGameScene(heapManager);
		treasures = factory->GetTreauseres();

		// 보물 위치에 보물 생성
		for (auto& treasure : treasures) {
			SpawnWorldItem(1000, treasure.treasure_pos);
		}
	}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		my_player->SetPosition(0.f, 1.0f, 0.f);
		auto m = my_player->GetComponent<CMovementComponent>();
		m->is_fly = true;
	}

	// 다우징 로드가 관리하는 treasuer_position(vector)에 보물 위치 정보를 넣는다.
	auto itemFinder = my_player->GetComponent<CItemFinder>();
	if (itemFinder) {
		itemFinder->RegisterTreasures(treasures);

		auto shaders = CSceneManager::GetInstance().GetShaders();
		auto mainCanvas = ui_manager->CreateCanvas();
		auto dowsingUI = std::make_shared<CUIDowsingArrow>(&itemFinder->angle);
		mainCanvas->AddChild(dowsingUI);
		dowsingUI->SetSize({ 64.0f, 64.0f });
		dowsingUI->SetEnable(false);
		// Material 설정
		std::shared_ptr<CMaterialComponent> m = std::make_shared<CMaterialComponent>();
		m->SetMaterial(factory->GetMaterial(shaders["ui"]->GetHeapManager(), "finder_arrow"));
		dowsingUI->SetMaterial(m);

		my_player->SetComponent(dowsingUI);
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
	};

	ProcessPickup();
}

void CGameScene::ProcessPickup()
{
	if (!my_player)
		return;

	if (CKeyManager::GetInstance().GetKeyState(KEY::Z) != KEY_STATE::TAP)
		return;

	XMFLOAT3 player_pos = my_player->GetPosition();

	std::vector<std::shared_ptr<CObject>> worldItems;;
	for (auto& obj : objects) {
		if (obj->GetObjectType() == OBJECT_TYPE::WORLD_ITEM) {
			worldItems.push_back(obj);
		}
	}

	auto it = std::find_if(worldItems.begin(), worldItems.end(),
		[&](const std::shared_ptr<CObject>& item) {
			XMFLOAT3 diff = Vector3::Subtract(item->GetPosition(), player_pos);
			return Vector3::Length(diff) <= PICKUP_RANGE;
		});

	if (it == worldItems.end())
		return;

	auto inv = my_player->GetInventory();
	if (inv) {
		auto worldItem = static_cast<CWorldItem*>(it->get());
		inv->AddItem(worldItem->GetItem());

		if (worldItem->GetItem()->GetItemType() == ITEM_TYPE::TREASURE) {
			XMFLOAT3 pos = worldItem->GetPosition();
			auto treasure_it = std::find_if(treasures.begin(), treasures.end(),
				[&pos](const TreasureInfo& info) {
					return info.treasure_pos.x == pos.x &&
					       info.treasure_pos.y == pos.y &&
					       info.treasure_pos.z == pos.z;
				});

			if (treasure_it != treasures.end())
				treasures.erase(treasure_it);

			my_player->GetComponent<CItemFinder>()->RegisterTreasures(treasures);
		}

		RemoveObject(worldItem->GetID());
	}
}

void CGameScene::SpawnWorldItem(int itemID, XMFLOAT3 position)
{
	auto itemData = ItemFactory::Create(itemID);

	auto item = std::make_shared<CWorldItem>(itemData);
	item->SetPosition(position);
	item->SetID(world_item_id_counter);
	item->Initialize(GET_DEVICE, GET_CMD_LIST);
	AddObject(item, world_item_id_counter);
	++world_item_id_counter;
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

	CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()["inst"]->GetHeapManager() };
	objects = factory->CreateGameSceneByServer(staticHeapManager, instance_data);
	treasures = factory->GetTreauseres();

	// 보물 위치에 보물 생성
	for (auto& treasure : treasures) {
		SpawnWorldItem(1000, treasure.treasure_pos);
	}
}
