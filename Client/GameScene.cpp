#include "stdafx.h"
#include "GameScene.h"
#include "SceneManager.h"
#include "PhysicsManager.h"
#include "GameFramework.h"

#include "Collider.h"
#include "MyPlayer.h"
#include "Camera.h"
#include "Shader.h"
#include "ObjectFactory.h"
#include "HumanMonster.h"
#include "Ghost.h"

#include "ItemFinder.h"
#include "ImGui/imgui.h"
#include "NetworkManager.h"
#include "ServerPacketHandler.h"
#include "User.h"

#include "KeyManager.h"

#include "Inventory.h"
#include "QuickSlot.h"
#include "WorldItem.h"
#include "WorldTool.h"			// (장비)파밍 도구
#include "MineableObject.h"
#include "Animator.h"
#include "WorldWeapon.h"		// (장비)무기
#include "WorldConsumable.h"	// 소비
#include "WorldOther.h"			// 기타
#include "WorldTreasure.h"		// 보물

#include "UIComponent.h"
#include "DataManager.h"

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
		//for (auto& treasure : treasures) {
		//	SpawnWorldItem(110, treasure.world_id, treasure.treasure_pos);
		//}

		factory->LoadItemFrame(heapManager);
	}

	// menu UI
	auto menuUI = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/Menu_UI.json");
	menuUI->SetEnable(false);
	ui_manager->AddCanvas(menuUI);
	// Player UI
	auto playerUI = ui_manager->GetDataManager()->LoadFromFile("../Modeling/UI/Player_UI.json");
	ui_manager->AddCanvas(playerUI);
	// player data와 연동
	auto hpBar = ui_manager->GetUI<CUIImage>("HP_UI");
	hpBar->BindFillAmount([this]() -> float {
		if (!my_player) return 0.0f;
		float current = static_cast<float>(my_player->GetHp());
		float max = static_cast<float>(my_player->GetMaxHp());
		return current / max;
		});
	auto energyBar = ui_manager->GetUI<CUIImage>("ENERGY_UI");
	energyBar->BindFillAmount([this]() -> float {
		if (!my_player) return 0.0f;
		float current = static_cast<float>(my_player->GetStamina());
		float max = static_cast<float>(my_player->GetMaxStamina());
		return current / max;
		});

	SetButtonEvents();

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
	SpawnWorldItem(40, XMFLOAT3{3, 2, 4});
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()["skinning"]->GetHeapManager() };
		my_player = factory->CreateMyPlayer(skinningHeapManager);
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
	if (KEY_TAP(KEY::ESC)) {
		auto menuUI = ui_manager->GetUI<CUICanvas>("LobbyMenuCanvas");
		if (menuUI) {
			ui_manager->ToggleUI("LobbyMenuCanvas", !menuUI->is_enable, menuUI->is_enable);
		}
	}

	CScene::Update(elapsedTime);

	ProcessPickup();

	if (my_player) {
		ProcessMining();
		my_player->BeginSendInputPacket(elapsedTime);
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

void CGameScene::SetButtonEvents()
{
	auto menuToCustomBtn = ui_manager->GetUI<CUIButton>("ToCustom");
	auto menuBackBtn = ui_manager->GetUI<CUIButton>("Back");

	if (menuToCustomBtn) {
		menuToCustomBtn->OnClick = [this]() {
			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
				ui_manager->ToggleUI("LobbyMenuCanvas", false, true);
			}
		};
	}

	if (menuBackBtn) {
		menuBackBtn->OnClick = [this]() {
			if (g_is_single) {
				CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::TITLE);
				ui_manager->ToggleUI("LobbyMenuCanvas", false, false);
			}
		};
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
			my_player->SetPosition(1.f, 5.0f, 1.5f);	// Lobby 위치 그대로 가져오는 거 방지

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

		// 보물이라면 
		if (worldItem->GetItem()->GetItemType() == ITEM_TYPE::TREASURE) {

			// 아이템 줍기 시도
			if (inv->AddItem(worldItem->GetItem())) {

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

				RemoveObject(worldItem->GetID());
			}
		}
		else {
			inv->AddItem(worldItem->GetItem());
			RemoveObject(worldItem->GetID());
		}
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

void CGameScene::ProcessMining()
{
	if (!my_player)
		return;

	bool is_digging = (my_player->GetState() == PLAYER_STATE::DIG);

	// DIG→IDLE 전이 감지 = 애니메이션 자연 완료(인터럽트 제외) → 데미지 처리
	if (was_digging && !is_digging && mining_target && my_player->GetDigAnimFinished()) {

		my_player->SetDigAnimFinished(false);

		bool still_exists = false;
		for (auto& obj : objects) {

			if (obj.get() == mining_target) { 
				still_exists = true; 
				break; 
			}
		}

		if (still_exists) {
			mining_target->TakeDamage();

			if (mining_target->IsDestroyed()) {
				CMineableObject* to_remove = mining_target;
				mining_target = nullptr;
				XMFLOAT3 pos = to_remove->GetPosition();

				auto it = std::find_if(objects.begin(), objects.end(),
					[to_remove](const std::shared_ptr<CObject>& obj) {
						return obj.get() == to_remove;
					});

				if (it != objects.end()) {

					if (auto col = to_remove->GetComponent<CColliderComponent>())
						CPhysicsManager::GetInstance().EraseCollider(col);

					size_t idx  = std::distance(objects.begin(), it);
					size_t last = objects.size() - 1;

					if (idx != last) {
						std::swap(objects[idx], objects[last]);
						uint64 moved_id = objects[idx]->GetID();
						auto map_it = id_To_Index.find(moved_id);
						if (map_it != id_To_Index.end())
							map_it->second = idx;
					}
					objects.pop_back();
				}

				SpawnWorldItem(110, pos);
			}
		}
		else {
			mining_target = nullptr;
		}
	}

	was_digging = is_digging;

	// 플레이어가 도구를 장착하고 있는지 검사
	auto qs = my_player->GetQuickSlot();
	bool has_tool = qs && qs->GetSelectedSubType() == ITEM_SUB_TYPE::TOOL;

	// 즉, IDLE 상태이고 좌클릭 눌렀고 (홀딩x), 도구 장착 시에만 채굴 애니메이션 재생
	if (KEY_TAP(KEY::LBTN) && !is_digging && has_tool && !ImGui::GetIO().WantCaptureMouse) {

		mining_target = nullptr;
		my_player->SetDigAnimFinished(false);

		XMFLOAT3 playerPos = my_player->GetPosition();
		float min_dist = MINING_RANGE;

		for (auto& obj : objects) {

			if (obj->GetObjectType() != OBJECT_TYPE::MINEABLE_OBJECT) 
				continue;

			float dist = Vector3::Length(Vector3::Subtract(obj->GetPosition(), playerPos));
			if (dist < min_dist) {
				min_dist = dist;
				mining_target = static_cast<CMineableObject*>(obj.get());
			}
		}

		my_player->SetState(PLAYER_STATE::DIG);
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

	worldItem->Initialize();

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

	worldItem->Initialize();

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

	// 보물 & 몬스터 spawn 위치들 모두 clear
	treasures.clear();
	humanMonster_spawn_positions.clear();
	ghost_spawn_positions.clear();

	CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()["inst"]->GetHeapManager() };
	objects = factory->CreateGameSceneByServer(staticHeapManager, instance_data);
}

void CGameScene::Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt)
{
	XMFLOAT3 pos{ pkt.x, pkt.y, pkt.z };

	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		SpawnWorldItem(pkt.item_id, pkt.item_world_id, pos);
		TreasureInfo treasure{ pkt.item_world_id, pos };
		treasures.push_back(treasure);
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

void CGameScene::Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt)
{
	if (!pkt.success)
		return;
}
