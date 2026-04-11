#include "stdafx.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "PhysicsManager.h"
#include "GameFramework.h"

#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "ObjectFactory.h"
#include "HumanMonster.h"
#include "Ghost.h"

#include "ItemFinder.h"
#include "NetworkManager.h"
#include "ServerPacketHandler.h"
#include "User.h"

#include "KeyManager.h"

#include "Inventory.h"
#include "QuickSlot.h"
#include "WorldItem.h"
#include "WorldTool.h"			// (장비)파밍 도구
#include "WorldWeapon.h"		// (장비)무기
#include "WorldConsumable.h"	// 소비
#include "WorldOther.h"			// 기타
#include "WorldTreasure.h"		// 보물

#include "UIComponent.h"
#include "Movement.h"
#include "Animator.h"

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

		humanMonster_spawn_positions = factory->GetHumanMonsterSpawnPositions();
		ghost_spawn_positions = factory->GetGhostSpawnPositions();

		// 보물 위치에 보물 생성
		// 보물 생성만 멀티용 SpawnWorldItem 함수 호출.
		for (auto& treasure : treasures) {
			SpawnWorldItem(110, treasure.world_id, treasure.treasure_pos);
		}

		factory->LoadItemFrame(heapManager);
	}

	// 아이템 생성 (테스트)
	SpawnWorldItem(5, XMFLOAT3{-1, 2, -1});
	SpawnWorldItem(9, XMFLOAT3{-1, 2, -2});
	SpawnWorldItem(14, XMFLOAT3{-1, 2, -3});
	SpawnWorldItem(15, XMFLOAT3{-1, 2, -4});
	SpawnWorldItem(17, XMFLOAT3{-2, 2, -1});
	SpawnWorldItem(19, XMFLOAT3{-2, 2, -2});

	SpawnWorldItem(20, XMFLOAT3{1, 2, 1});
	SpawnWorldItem(21, XMFLOAT3{1, 2, 2});
	SpawnWorldItem(22, XMFLOAT3{1, 2, 3});
	SpawnWorldItem(23, XMFLOAT3{1, 2, 4});
	SpawnWorldItem(24, XMFLOAT3{2, 2, 1});
	SpawnWorldItem(25, XMFLOAT3{2, 2, 2});
	SpawnWorldItem(26, XMFLOAT3{2, 2, 3});
	SpawnWorldItem(27, XMFLOAT3{2, 2, 4});
	SpawnWorldItem(29, XMFLOAT3{3, 2, 1});
	SpawnWorldItem(30, XMFLOAT3{3, 2, 2});
	SpawnWorldItem(31, XMFLOAT3{3, 2, 3});
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
		auto m = my_player->GetComponent<CMovementComponent>();
		m->is_fly = true;
		my_player->SetPosition(1.f, 2.0f, 1.f);
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

	// 싱글 드롭 콜백 등록
	if (my_player) {
		auto inv = my_player->GetInventory();
		if (inv) {
			inv->SetDropCallback([this](std::shared_ptr<CItem> item) {
				DropItemAtPlayerFeet(item);
				});
		}
	}

	// 싱글 전용: 몬스터 스폰
	if (g_is_single) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };

		for (const auto& pos : humanMonster_spawn_positions) {
			auto humanMonster = factory->CreateMonster(skinningHeapManager, MON_TYPE::HUMAN_MONSTER, scene_type);
			if (!humanMonster)
				continue;

			humanMonster->SetPosition(pos.x, 0.1f, pos.z);
			humanMonster->SetOriginPos({ pos.x, 0.1f, pos.z });
			AddObject(humanMonster, humanMonster->GetID());
		}

		for (const auto& pos : ghost_spawn_positions) {
			auto ghost = factory->CreateMonster(skinningHeapManager, MON_TYPE::GHOST, scene_type);
			if (!ghost)
				continue;

			ghost->SetPosition(pos.x, 0.1f, pos.z);
			ghost->SetOriginPos({ pos.x, 0.1f, pos.z });
			AddObject(ghost, ghost->GetID());
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

	ProcessPickup();

	if (my_player) {
		my_player->BeginSendInputPacket(elapsedTime);
		// 디버깅용 임시 설정
		if (KEY_PRESSED(KEY::U)) {
			CMovementComponent* m = my_player->GetComponent<CMovementComponent>();
			m->is_fly = !m->is_fly;
		}
	};
}

void CGameScene::DrawUI()
{
	if (my_player) {

		// 인벤토리
		auto inventory = my_player->GetInventory();
		if (inventory);
			inventory->Draw();

		// 퀵슬롯
		auto quick_slot = my_player->GetQuickSlot();
		if (quick_slot)
			quick_slot->Draw();
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
		if(g_is_single)
			my_player->SetPosition(1.f, 5.0f, 1.f);	// Lobby 위치 그대로 가져오는 거 방지

		// 다우징 로드가 관리하는 treasuer_position(vector)에 보물 위치 정보를 넣는다.
		auto itemFinder = my_player->GetComponent<CItemFinder>();
		if (itemFinder) {
			itemFinder->RegisterTreasures(treasures);
		}
	}
}

void CGameScene::ProcessPickup()
{
	if (!my_player)
		return;

	if (CKeyManager::GetInstance().GetKeyState(KEY::Z) != KEY_STATE::TAP)
		return;

	XMFLOAT3 playerPos = my_player->GetPosition();

	// Objects에는 플레이어, 몬스터, 맵 오브젝트, 아이템 모두 들어있다.
	// 여기서 아이템만 필터링한다.
	std::vector<std::shared_ptr<CObject>> worldItems;
	for (auto& obj : objects) {
		if (obj->GetObjectType() == OBJECT_TYPE::WORLD_ITEM) {
			worldItems.push_back(obj);
		}
	}

	// 플레이어 근처에 있는 아이템을 찾는다.
	auto it = std::find_if(worldItems.begin(), worldItems.end(),
		[&](const std::shared_ptr<CObject>& item) {
			XMFLOAT3 diff = Vector3::Subtract(item->GetPosition(), playerPos);
			return Vector3::Length(diff) <= PICKUP_RANGE;
		});

	if (it == worldItems.end())
		return;

	auto worldItem = static_cast<CWorldItem*>(it->get());

	// 싱글환경
	if (g_is_single) {

		auto inv = my_player->GetInventory();
		if (!inv)
			return;

		// 인벤토리에 아이템을 넣는다.
		inv->AddItem(worldItem->GetItem());

		// 보물이라면 
		if (worldItem->GetItem()->GetItemType() == ITEM_TYPE::TREASURE) {

			// 보물의 위치정보를 담고있는 벡터에서 찾은 보물을 삭제한다.
			uint32 id = worldItem->GetID();
			auto treasure_it = std::find_if(treasures.begin(), treasures.end(),
				[id](const TreasureInfo& info) {
					return info.world_id == id;
				});

			if (treasure_it != treasures.end())
				treasures.erase(treasure_it);

			// 다우징로드에 있는 보물 컨테이너 갱신
			// 다우징로드가 찾은 보물을 더이상 추적하지 말아야 하기 때문이다.
			my_player->GetComponent<CItemFinder>()->RegisterTreasures(treasures);
		}

		RemoveObject(worldItem->GetID());
	}
	else {
		// (멀티) 서버에 줍기 요청만 보낸다.
		// 인벤토리 추가/오브젝트 제거는 서버 응답(S_AddItem, S_DeSpawnItem)에서 처리.
		C_PickupItem pickupPkt;
		pickupPkt.player_id = my_player->GetUser()->GetUserID();
		pickupPkt.item_world_id = worldItem->GetID();
		pickupPkt.item_type = worldItem->GetItem()->GetItemType();
		pickupPkt.scene_type = my_player->GetCurrentSceneType();

		auto sendBuffer = MAKE_SEND_BUFFER(pickupPkt);
		my_player->GetSession()->DoSend(sendBuffer);
	}
}

// 싱글환경
void CGameScene::SpawnWorldItem(uint16 itemID, XMFLOAT3 position)
{
	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager{ shaders["inst"]->GetHeapManager() };

	auto worldItem = factory->CreateWorldItem(itemID, heapManager);
	if (!worldItem)
		return;

	worldItem->Initialize(GET_DEVICE, GET_CMD_LIST);

	// 아이템의 위치
	worldItem->SetPosition(position);

	// 아이템의 ID (CObject 클래스에 정의된 obj_id)
	worldItem->SetID(world_item_id_counter);

	// Scene의 objects 컨테이너에 추가
	AddObject(worldItem, world_item_id_counter);

	++world_item_id_counter;
}

// 멀티환경
void CGameScene::SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position)
{
	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager{ shaders["inst"]->GetHeapManager() };

	auto worldItem = factory->CreateWorldItem(itemID, heapManager);
	if (!worldItem)
		return;

	worldItem->Initialize(GET_DEVICE, GET_CMD_LIST);

	// 아이템의 위치
	worldItem->SetPosition(position);

	// 아이템의 ID (CObject 클래스에 정의된 obj_id)
	worldItem->SetID(itemWorldId);

	// Scene의 objects 컨테이너에 추가
	AddObject(worldItem, itemWorldId);
}

void CGameScene::DropItemAtPlayerFeet(std::shared_ptr<CItem> item)
{
	if (!my_player)
		return;

	XMFLOAT3 pos    = my_player->GetPosition();
	float    yawRad = XMConvertToRadians(my_player->GetYaw());
	pos.x          += sinf(yawRad) * 0.4f;
	pos.z          += cosf(yawRad) * 0.4f;
	pos.y           = max(pos.y, 0.0f);
	uint32   worldId = world_item_id_counter; // SpawnWorldItem 호출 전에 캡처

	SpawnWorldItem(item->GetItemId(), pos);   // 내부에서 world_item_id_counter 증가

	if (item->GetItemType() == ITEM_TYPE::TREASURE) {
		TreasureInfo info{ worldId, pos };
		treasures.push_back(info);
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

	// 몬스터 spawn 위치들 모두 clear
	humanMonster_spawn_positions.clear();
	ghost_spawn_positions.clear();

	CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()["inst"]->GetHeapManager() };
	objects = factory->CreateGameSceneByServer(staticHeapManager, instance_data);
	treasures.clear();
}

void CGameScene::Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt)
{
	XMFLOAT3 pos{ pkt.x, pkt.y, pkt.z };

	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		SpawnWorldItem(pkt.item_id, pkt.item_world_id, pos);
		TreasureInfo treasure{ pkt.item_world_id, pos };
		treasures.push_back(treasure);

		if (my_player) {
			auto itemFinder = my_player->GetComponent<CItemFinder>();
			if (itemFinder) {
				itemFinder->AddTreasure(treasure);
			}
		}
	}
	else {
		SpawnWorldItem(pkt.item_id, pkt.item_world_id, pos);
	}
}

// 아이템 리스트 (가변인자)
void CGameScene::Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Item_List& pkt)
{
	S_Item_List::ItemList itemList = pkt.GetItemList();

	for (uint32 i = 0; i < pkt.item_count; ++i) {

		XMFLOAT3 pos{ itemList[i].x,  itemList[i].y, itemList[i].z};

		if (itemList[i].item_type == ITEM_TYPE::TREASURE) {

			// 아마 나중에 이부분은 삭제할 수도 있다. 보물 파밍 메커니즘 상의
			SpawnWorldItem(itemList[i].item_id, itemList[i].item_world_id, pos);
			TreasureInfo treasure{ itemList[i].item_world_id, pos };
			treasures.push_back(treasure);

			if (my_player) {
				auto itemFinder = my_player->GetComponent<CItemFinder>();
				if (itemFinder) {
					itemFinder->AddTreasure(treasure);
				}
			}
		}
		else {
			SpawnWorldItem(itemList[i].item_id, itemList[i].item_world_id, pos);
		}
	}
}

void CGameScene::Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt)
{
	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		auto treasureInfo = std::find_if(treasures.begin(), treasures.end(), [&](const TreasureInfo& info) {
			return info.world_id == pkt.item_world_id;
			});

		if (treasureInfo != treasures.end()) {
			treasures.erase(treasureInfo);

			if (my_player) {
				auto itemFinder = my_player->GetComponent<CItemFinder>();
				if (itemFinder)
					itemFinder->RegisterTreasures(treasures);
			}
		}

		RemoveObject(pkt.item_world_id);
	}
	else {
		auto it = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
			return obj->GetID() == pkt.item_world_id;
			});

		if (it != objects.end()) {
			RemoveObject(pkt.item_world_id);
		}
	}
}

void CGameScene::Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt)
{
	if (pkt.item_type == ITEM_TYPE::TREASURE) {
		auto treasure = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
			return obj->GetID() == pkt.item_world_id;
			});

		if (treasure == objects.end())
			return;

		auto worldTreasure = static_cast<CWorldTreasure*>(treasure->get());
		my_player->GetInventory()->AddItemWithId(worldTreasure->GetItem(), pkt.inventory_id);
	}
	else {
		auto item = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
			return obj->GetID() == pkt.item_world_id;
			});

		if (item == objects.end())
			return;

		auto worldItem = static_cast<CWorldItem*>(item->get());
		my_player->GetInventory()->AddItemWithId(worldItem->GetItem(), pkt.inventory_id);
	}
}

void CGameScene::Handle_S_RemoveItem(std::shared_ptr<Session> session, const S_RemoveItem& pkt)
{
	if (!my_player)
		return;

	my_player->GetInventory()->RemoveItem(pkt.inventory_id);
}

void CGameScene::Handle_S_EquipItem(std::shared_ptr<Session>& session, const S_EquipItem& pkt)
{
	if (my_player->GetID() == pkt.player_id) {
		my_player->SetEquippedItemId(pkt.item_id);
	}
	else {
		auto& indexMap = GetIDIndex();
		auto it = indexMap.find(pkt.player_id);
		if (it == indexMap.end()) 
			return;

		auto player = std::dynamic_pointer_cast<CPlayer>(objects[it->second]);
		if (player)
			player->SetEquippedItemId(pkt.item_id);
	}
}
