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
	scene_bounds.Center = XMFLOAT3{ MapGenerator::WIDTH * 2 / 2, 0.0f, MapGenerator::HEIGHT * 2 / 2 };
	scene_bounds.Radius = MapGenerator::HEIGHT * 2 / 2;
}

CGameScene::~CGameScene()
{
}

void CGameScene::Initialize()
{
	auto shaders = CSceneManager::GetInstance().GetShaders();

	if (objects.empty()) {
		CDescriptorHeapManager* heapManager{ shaders[EShaderName::Inst]->GetHeapManager() };
		objects = factory->CreateGameScene(heapManager);
		treasures = factory->GetTreauseres();

		humanMonster_spawn_positions = factory->GetHumanMonsterSpawnPositions();
		ghost_spawn_positions = factory->GetGhostSpawnPositions();

		factory->LoadItemFrame(heapManager);
		// 우선 게임씬에서 load
		factory->LoadTwoSideFrame(shaders[EShaderName::TwoSide]->GetHeapManager());
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
	SpawnWorldItem(5, XMFLOAT3{ -1, 2, -1 });
	SpawnWorldItem(9, XMFLOAT3{ -1, 2, -2 });
	SpawnWorldItem(14, XMFLOAT3{ -1, 2, -3 });
	SpawnWorldItem(15, XMFLOAT3{ -1, 2, -4 });
	SpawnWorldItem(17, XMFLOAT3{ -2, 2, -1 });
	SpawnWorldItem(19, XMFLOAT3{ -2, 2, -2 });

	SpawnWorldItem(20, XMFLOAT3{ 1, 2, 1 });
	SpawnWorldItem(21, XMFLOAT3{ 1, 2, 2 });
	SpawnWorldItem(22, XMFLOAT3{ 1, 2, 3 });
	SpawnWorldItem(23, XMFLOAT3{ 1, 2, 4 });
	SpawnWorldItem(24, XMFLOAT3{ 2, 2, 1 });
	SpawnWorldItem(25, XMFLOAT3{ 2, 2, 2 });
	SpawnWorldItem(26, XMFLOAT3{ 2, 2, 3 });
	SpawnWorldItem(27, XMFLOAT3{ 2, 2, 4 });
	SpawnWorldItem(29, XMFLOAT3{ 3, 2, 1 });
	SpawnWorldItem(30, XMFLOAT3{ 3, 2, 2 });
	SpawnWorldItem(31, XMFLOAT3{ 3, 2, 3 });
	SpawnWorldItem(40, XMFLOAT3{ 3, 2, 4 });
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()[EShaderName::Skinning]->GetHeapManager() };
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
		m->SetMaterial(factory->GetMaterial(shaders[EShaderName::UI]->GetHeapManager(), "finder_arrow"));
		dowsingUI->SetMaterial(m);

		my_player->SetComponent(dowsingUI);
	}

	// 싱글 전용: 드롭 콜백 등록
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
		CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()[EShaderName::Skinning]->GetHeapManager() };
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
	// 싱글 전용 
	if (!g_is_single)
		return;

	if (!my_player)
		return;

	bool isDigging = (my_player->GetState() == PLAYER_STATE::DIG);

	// DIG→IDLE 전이 감지 = 애니메이션 1회 재생 완료(인터럽트 제외) → 데미지 처리
	if (was_digging && !isDigging && mining_target && my_player->GetDigAnimFinished()) {

		my_player->SetDigAnimFinished(false);

		bool stillExist = false;
		for (auto& obj : objects) {

			if (obj.get() == mining_target) { 
				stillExist = true;
				break; 
			}
		}

		if (stillExist) {

			// 채굴 가능한 오브젝트에 데미지를 입힌다.
			mining_target->TakeDamage();

			// 플레이어의 도구 내구도를 감소 시킨다. 
			auto qs = my_player->GetQuickSlot();
			auto inv = my_player->GetInventory();
			bool isTool = inv && qs && (qs->GetSelectedSubType() == ITEM_SUB_TYPE::TOOL);
			
			if (isTool) {
				const auto& items = inv->GetItems();
				auto it = items.find(qs->GetSelectedInvId());
				if (it != items.end())
					if (auto tool = dynamic_cast<CTool*>(it->second.get())) {

						// 도구 내구도 닳기
						tool->ReduceDurability();

						// 내구도가 0이 되어 파괴되었는지 체크
						if (tool->GetCurrentDurability() <= 0) {
							inv->RemoveItem(qs->GetSelectedInvId());
						}
					}
			}

			if (mining_target->IsDestroyed()) {
				CMineableObject* toRemove = mining_target;
				mining_target = nullptr;
				XMFLOAT3 pos = toRemove->GetPosition();

				auto it = std::find_if(objects.begin(), objects.end(),
					[toRemove](const std::shared_ptr<CObject>& obj) {
						return obj.get() == toRemove;
					});

				if (it != objects.end()) {

					if (auto col = toRemove->GetComponent<CColliderComponent>())
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

	was_digging = isDigging;

	// 플레이어가 도구를 장착하고 있는지 검사
	auto qs = my_player->GetQuickSlot();
	bool hasTool = qs && qs->GetSelectedSubType() == ITEM_SUB_TYPE::TOOL;

	// 이동 중에는 채굴 시작 불가 
	bool isMoving = KEY_PRESSED(KEY::W) || KEY_PRESSED(KEY::A)
	              || KEY_PRESSED(KEY::S) || KEY_PRESSED(KEY::D);

	// 즉, IDLE 상태이고 좌클릭 눌렀고 (홀딩x), 도구 장착 시에만 채굴 애니메이션 재생
	if (KEY_TAP(KEY::LBTN) && !isDigging && hasTool && !ImGui::GetIO().WantCaptureMouse && !isMoving) {

		mining_target = nullptr;
		my_player->SetDigAnimFinished(false);

		XMFLOAT3 playerPos = my_player->GetPosition();
		float minDist = MINING_RANGE;

		for (auto& obj : objects) {

			if (obj->GetObjectType() != OBJECT_TYPE::MINEABLE_OBJECT) 
				continue;

			float dist = Vector3::Length(Vector3::Subtract(obj->GetPosition(), playerPos));
			if (dist < minDist) {
				minDist = dist;
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
	CDescriptorHeapManager* heapManager{ shaders[EShaderName::Inst]->GetHeapManager() };

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
	CDescriptorHeapManager* heapManager{ shaders[EShaderName::Inst]->GetHeapManager() };

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

	CDescriptorHeapManager* staticHeapManager{ CSceneManager::GetInstance().GetShaders()[EShaderName::Inst]->GetHeapManager() };
	objects = factory->CreateGameSceneByServer(staticHeapManager, instance_data);
}

void CGameScene::Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt)
{
	XMFLOAT3 pos{ pkt.x, pkt.y, pkt.z };
	SpawnWorldItem(pkt.item_id, pkt.item_world_id, pos);
}

// 아이템 리스트 (가변인자)
void CGameScene::Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Item_List& pkt)
{
	S_Item_List::ItemList itemList = pkt.GetItemList();

	for (uint32 i = 0; i < pkt.item_count; ++i) {

		XMFLOAT3 pos{ itemList[i].x, itemList[i].y, itemList[i].z };
		SpawnWorldItem(itemList[i].item_id, itemList[i].item_world_id, pos);
	}
}

void CGameScene::Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt)
{
	auto it = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
		return obj->GetID() == pkt.item_world_id;
		});

	if (it != objects.end()) {
		RemoveObject(pkt.item_world_id);
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

		bool wasDowsing = my_player->GetDowsing();
		auto itemFinder = my_player->GetComponent<CItemFinder>();
		auto animator = my_player->GetComponent<CAnimatorComponent>();

		if (!itemFinder || !animator)
			return;

		if (!wasDowsing && pkt.is_dowsing_rod && pkt.item_id < 0) {
			my_player->SetDowsing(true);

			itemFinder->Toggle();

			animator->PlayAction("Ganga_search");
		}
		else if (wasDowsing && !pkt.is_dowsing_rod && pkt.item_id < 0) {
			my_player->SetDowsing(false);

			itemFinder->Toggle();

			animator->PlayAction("");
		}
		else {
			my_player->SetEquippedItemId(pkt.item_id);
		}
	}
	// 다른 플레이어
	else {
		auto& indexMap = GetIDIndex();
		auto it = indexMap.find(pkt.player_id);
		if (it == indexMap.end()) 
			return;

		auto player = std::dynamic_pointer_cast<CPlayer>(objects[it->second]);
		if (!player)
			return;

		auto animator = player->GetComponent<CAnimatorComponent>();
		if (!animator)
			return;

		bool wasDowsing = player->GetDowsing();

		if (!wasDowsing && pkt.is_dowsing_rod && pkt.item_id < 0){
			player->SetDowsing(true);
			animator->PlayAction("Ganga_search");
		}
		else if (wasDowsing && !pkt.is_dowsing_rod && pkt.item_id < 0) {
			player->SetDowsing(false);
			animator->PlayAction("");
		}
		else {
			player->SetEquippedItemId(pkt.item_id);
		}
	}
}

void CGameScene::Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt)
{
	if (!pkt.success)
		return;
}

void CGameScene::Handle_S_MineableList(std::shared_ptr<Session>& session, S_MineableList& pkt)
{
	// CreateGameSceneByServer 함수에서 CMineableObject를 생성했다.
	// 여기서는 생성된 CMineableObject를 찾아서 server world_id를 세팅하고,
	// 다우징로드가 참조하는 treasures에도 등록한다.

	S_MineableList::MineableList mineableList = pkt.GetMineableList();

	for (uint32 i = 0; i < pkt.mineable_count; ++i) {

		XMFLOAT3 pos{ mineableList[i].x, mineableList[i].y, mineableList[i].z };

		for (auto& obj : objects) {
			if (obj->GetObjectType() != OBJECT_TYPE::MINEABLE_OBJECT)
				continue;

			const XMFLOAT3& p = obj->GetPosition();
			if (p.x == pos.x && p.y == pos.y && p.z == pos.z) {
				obj->SetID(mineableList[i].world_id);
				break;
			}
		}

		treasures.push_back(TreasureInfo{ mineableList[i].world_id, pos });
	}

	// RegisterTreasures는 이후 GameScene::Enter → BuildObjects에서 호출됨 (my_player 유효 시점)
}

void CGameScene::Handle_S_DestroyMineable(std::shared_ptr<Session>& session, const S_DestroyMineable& pkt)
{
	// CMineableObject는 id_To_Index 미등록이라 RemoveObject 대신 직접 제거
	auto objIt = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
		return obj->GetObjectType() == OBJECT_TYPE::MINEABLE_OBJECT
			&& obj->GetID() == pkt.obj_id;
		});

	if (objIt != objects.end()) {
		size_t idx  = std::distance(objects.begin(), objIt);
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

	// 다우징로드가 관리하는 treasures에서도 제거 후 갱신
	auto treasureIt = std::find_if(treasures.begin(), treasures.end(), [&](const TreasureInfo& info) {
		return info.world_id == static_cast<uint32>(pkt.obj_id);
		});

	if (treasureIt != treasures.end()) {
		treasures.erase(treasureIt);

		if (my_player) {
			if (auto itemFinder = my_player->GetComponent<CItemFinder>())
				itemFinder->RegisterTreasures(treasures);
		}
	}
}

void CGameScene::Handle_S_UpdateDurability(std::shared_ptr<Session>& session, const S_UpdateDurability& pkt)
{
	if (my_player->GetID() == pkt.player_id) {
		auto inv = my_player->GetInventory();
		if (inv) {
			const auto& items = inv->GetItems();
			auto it = items.find(pkt.inventory_id);
			if (it == items.end())
				return;

			if (pkt.item_type == ITEM_TYPE::EQUIPMENT && pkt.item_sub_type == ITEM_SUB_TYPE::TOOL) {
				auto tool = std::static_pointer_cast<CTool>(it->second);
				tool->SetCurrentDurability(pkt.current_durability);
			}
		}
	}
}
