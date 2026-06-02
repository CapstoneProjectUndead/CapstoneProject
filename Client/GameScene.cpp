#include "stdafx.h"
#include <cstdio>
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
#include "DogMonster.h"
#include "AIComponent.h"

#include "ItemFinder.h"
#include "ImGui/imgui.h"
#include "ImGuiManager.h"
#include "ResourceManager.h"
#include "NetworkManager.h"
#include "ServerPacketHandler.h"
#include "User.h"

#include "KeyManager.h"

#include "Inventory.h"
#include "QuickSlot.h"
#include "WorldItem.h"
#include "Shop.h"
#include "WorldTool.h"			// (장비)파밍 도구
#include "MineableObject.h"
#include "Animator.h"
#include "WorldWeapon.h"		// (장비)무기
#include "WorldConsumable.h"	// 소비
#include "WorldOther.h"			// 기타
#include "WorldTreasure.h"		// 보물

#include "UIComponent.h"
#include "DataManager.h"
#include "SoundManager.h"
#include "ItemFactory.h"
#include "MapUtils.h"

#undef min
#undef max

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
	if (objects.empty()) {
		objects = factory->CreateGameScene();
		treasures = factory->GetTreauseres();

		for (const auto& pos : factory->GetHumanMonsterSpawnPositions())
			monster_spawn_info.push_back({ pos, MON_TYPE::HUMAN_MONSTER, 0.f, -1.f, {} });
		for (const auto& pos : factory->GetGhostSpawnPositions())
			monster_spawn_info.push_back({ pos, MON_TYPE::GHOST, 0.f, -1.f, {} });
		for (const auto& pos : factory->GetDogMonsterSpawnPositions())
			monster_spawn_info.push_back({ pos, MON_TYPE::ANIMAL_MONSTER, 0.f, -1.f, {} });

		factory->LoadItemFrame();
		// 우선 게임씬에서 load
		factory->LoadTwoSideFrame();
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
	SpawnWorldItem(1, XMFLOAT3{ -1, 2, -1 });
	SpawnWorldItem(5, XMFLOAT3{ -1, 2, -1 });
	SpawnWorldItem(9, XMFLOAT3{ -1, 2, -2 });
	SpawnWorldItem(14, XMFLOAT3{ -1, 2, -3 });
	SpawnWorldItem(15, XMFLOAT3{ -1, 2, -4 });
	SpawnWorldItem(16, XMFLOAT3{ 1, 2, 1 });
	SpawnWorldItem(17, XMFLOAT3{ -2, 2, -1 });
	SpawnWorldItem(19, XMFLOAT3{ -2, 2, -2 });

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

	// 테스트 (임시코드)
	//CDescriptorHeapManager* skinningHeapManager{ CSceneManager::GetInstance().GetShaders()[EShaderName::Skinning]->GetHeapManager() };
	//auto dog = factory->CreateMonster(skinningHeapManager, MON_TYPE::ANIMAL_MONSTER, scene_type);
	//if (dog) {
	//	dog->SetPosition(2, 0.1f, 2.f);
	//	dog->SetOriginPos({ 2, 0.1f, 2.f });
	//	AddObject(dog, dog->GetID());
	//}
}

void CGameScene::BuildObjects(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// 플레이어 생성
	if (!my_player) {
		my_player = factory->CreateMyPlayer();
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
		m->SetMaterial(factory->GetMaterial("finder_arrow", EShaderName::UI));
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
		for (auto& info : monster_spawn_info) {
			auto monster = factory->CreateMonster(info.type, scene_type);
			if (!monster)
				continue;
		
			monster->SetPosition(info.position.x, 0.1f, info.position.z);
			monster->SetOriginPos(info.position);
		
			// 콜백함수 등록
			if (info.type == MON_TYPE::HUMAN_MONSTER) {
				if (auto human = std::dynamic_pointer_cast<CHumanMonster>(monster)) {
					human->SetSpawnCallback([this](MON_TYPE type, XMFLOAT3 pos) {
						auto dog = factory->CreateMonster(type, scene_type);
						if (!dog)
							return;
						dog->SetPosition(pos.x, pos.y, pos.z);
						dog->SetOriginPos(pos);
						AddObject(dog, dog->GetID());
						// ApplySeparation이 monster_spawn_info를 순회하므로 소환몹도 등록
						// 마지막 인자 true = 1회성 소환 (사망 시 리스폰 대신 entry 제거)
						monster_spawn_info.push_back({ pos, type, 0.f, -1.f, dog, true });
					});
				}
			}

			AddObject(monster, monster->GetID());
			info.monster = monster;
			info.respawn_time = monster->GetRespawnTime();
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

	// DEAD 진입 시 Player_UI 캔버스 끄기 (HP/스태미나/가방/조준점 일괄)
	if (my_player && my_player->GetState() == PLAYER_STATE::DEAD && !player_ui_disabled) {
		ui_manager->ToggleUI("Player_UI", false, true);
		player_ui_disabled = true;
	}

	// 라운드 타이머 진행
	// - 싱글: 클라가 자체 카운트
	// - 멀티: 서버가 매 틱 S_PlayerMove에 round_timer를 실어 보내므로 Handle_S_Move_Player가 덮어씀
	if (g_is_single && round_active && round_timer > 0.f) {
		round_timer -= elapsedTime;

		if (round_timer <= 60.f) {
			// 좌표/범위는 Enter()에서 이미 확정됨. 여기선 활성화만 트리거 (노란 원 표시 시작)
			return_active = true;
		}

		if (round_timer <= 0.f || my_player->GetReturned()
			|| my_player->GetState() == PLAYER_STATE::DEAD) {
			round_timer = 0.f;
			CKeyManager::GetInstance().SetMouseMode(false);
			TriggerSinglePlayerSettlement();
			CSoundManager::GetInstance().Play(SOUND_ID::Settlement);
		}
	}

	CScene::Update(elapsedTime);

	// 빈사/관전 3인칭 카메라가 벽을 뚫고 들어가지 않도록 한다.
	// CScene::Update 이후에 실행되어, 이 프레임의 카메라 위치가 확정된 뒤 보정한다.
	ApplyCameraCollision();

	if (g_is_single) {
		if (!show_settlement_modal)
			UpdateMonsters(elapsedTime);
		DetectMyPlayerReturn(); // 싱글 전용: 복귀존 도달 감지 (멀티는 서버 권위)
	}

	if (my_player) {

		// 아이템 줍기
		ProcessPickup();

		// 플레이어가 장착하고 있는 아이템에 따라 행동 분기
		auto qs = my_player->GetQuickSlot();
		bool hasTool = qs && qs->GetSelectedSubType() == ITEM_SUB_TYPE::TOOL;
		bool isBareHand = (my_player->GetEquippedItemId() == 0);
		bool hasWeapon = qs && (qs->GetSelectedSubType() == ITEM_SUB_TYPE::MELEE_WEAPON
			|| qs->GetSelectedSubType() == ITEM_SUB_TYPE::RANGED_WEAPON);

		if (!ImGui::GetIO().WantCaptureMouse && (hasTool || isBareHand) 
			&& !my_player->GetIsPossessed()
			&& !my_player->GetIsStunned()
			&& !my_player->GetIsKnockedBack()) {

			uint16 equippedId = my_player->GetEquippedItemId();
			bool isShovel = equippedId >= 1 && equippedId <= 4;

			if (isBareHand || isShovel) {
				// 보이지 않는 보물 파밍
				ProcessUnVisibleObjectMining(elapsedTime);
			}
			else {
				// 보이는 보물 파밍
				ProcessVisibleObjectMining(elapsedTime);
			}
		}
		else if(!ImGui::GetIO().WantCaptureMouse && hasWeapon) {
			ProcessAttack(elapsedTime);
		}

		my_player->BeginSendInputPacket(elapsedTime);

		// (멀티 전용) 빙의 해제 / 빈사 소생 progress bar UI는 클라이언트 예측 기법 적용
		// 본인이 빙의/빈사 중이면 C-홀드 액션 불가
		if (!g_is_single && !my_player->GetDowsing() && !my_player->GetIsPossessed() && !my_player->IsIncapacitated()) {
			UpdateCHoldAction(elapsedTime);
		}
	};
}

// 빈사/관전 3인칭 카메라가 벽을 뚫지 않도록, 머리->카메라 사이의 벽까지 거리를 재서 카메라를 앞으로 당긴다.
// (정적 오브젝트의 로컬 AABB를 월드 변환해 레이 검사 - 콜라이더에 의존하지 않으므로 멀티에서도 동작)
void CGameScene::ApplyCameraCollision()
{
	if (!camera || !camera->NeedsCollisionCheck())
		return;

	XMFLOAT3 target = camera->GetLookAtPoint();   // player's head being viewed
	XMFLOAT3 camPos = camera->GetPos();

	XMVECTOR vTarget = XMLoadFloat3(&target);
	XMVECTOR vDir    = XMVectorSubtract(XMLoadFloat3(&camPos), vTarget);
	float desiredDist = XMVectorGetX(XMVector3Length(vDir));
	if (desiredDist < 0.0001f)
		return;
	vDir = XMVector3Normalize(vDir);

	float closest = desiredDist;
	for (const auto& obj : objects) {
		if (obj->GetObjectType() != OBJECT_TYPE::STATIC_OBJECT)
			continue;

		BoundingBox local = obj->GetLocalAABB();
		// skip objects without a usable AABB (e.g. meshless static objects)
		if (local.Extents.x <= 0.0f && local.Extents.y <= 0.0f && local.Extents.z <= 0.0f)
			continue;

		BoundingBox box;
		local.Transform(box, XMLoadFloat4x4(&obj->world_matrix));

		float hitDist = 0.0f;
		if (box.Intersects(vTarget, vDir, hitDist) && hitDist < closest)
			closest = hitDist;
	}

	if (closest < desiredDist) {
		// keep a small gap from the wall, and never collapse fully onto the head
		constexpr float WALL_MARGIN = 0.15f;
		constexpr float MIN_DIST    = 0.2f;
		float safe = std::max(closest - WALL_MARGIN, MIN_DIST);
		camera->PullInToDistance(target, safe);
	}
}

void CGameScene::DrawUI()
{
	// 라운드 타이머 오버레이 (화면 상단 중앙)
	if (round_active) {
		CImGuiManager::DrawRoundTimer(round_timer);
	}

	// 복귀존 마커 (월드 원형 링, 60초 시점 활성화 이후)
	if (return_active && camera) {
		DrawReturnMarker();
	}

	// 복귀 토스트 (S_PlayerReturned 받을 때마다 누적, 2.5초 자동 만료)
	DrawReturnToasts();

	// 정산 결과 모달
	DrawSettlementModal();

	if (my_player) {

		// 인벤토리
		auto inventory = my_player->GetInventory();
		if (inventory);
			inventory->Draw();

		// 퀵슬롯
		auto quick_slot = my_player->GetQuickSlot();
		if (quick_slot && (my_player->GetState() != PLAYER_STATE::DEAD))
			quick_slot->Draw();

		// 빙의 해제 / 빈사 소생 진행 바 (멀티 전용) — 대상 종류에 따라 다른 UI
		if (!g_is_single && my_player->GetCHoldProgress() > 0.0f) {
			CSoundManager::GetInstance().Play(SOUND_ID::clock_alarm, 0);
			if (my_player->GetCHoldTarget() == CHOLD_TARGET::RESCUE)
				DrawRescueProgressBar();
			else
				DrawDePossessProgressBar();
		}

		// 구조 포기 버튼 (멀티 전용, 빈사 본인 화면에 표시)
		DrawGiveUpButton();
	}

	// 상대 플레이어 상태 UI (오른쪽 상단)
	DrawOpponentStatus();
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

	// 라운드 타이머 시작
	// - 싱글: 클라가 즉시 ROUND_DURATION으로 시작 (자체 카운트)
	// - 멀티: 서버 권위. 첫 S_PlayerMove의 round_timer가 도착하면 SetRoundTimer가 활성화.
	if (g_is_single) {
		round_timer  = ROUND_DURATION;
		round_active = true;
	}

	// 복귀존 상태 리셋 (재진입 시 이전 라운드 잔여값 제거) (싱글/멀티 모두 유효)
	return_active = false;
	return_center = {};
	return_toasts.clear();
	my_player->SetReturned(false);

	// 복귀 지점 = 맨홀 위치 (MapGenerator::PlaceManhole이 결정). 라운드 시작 시 1회 확정.
	// 활성화(return_active)는 60초 전 Update에서 트리거됨. 멀티는 서버 S_ReturnZoneActive로 받음.
	if (g_is_single) {
		return_center = MapGenerator::GetManholePosition();
		return_range  = RETURN_RANGE;
	}

	// 정산 모달 리셋
	show_settlement_modal = false;
	settlement_result     = SettlementResult{};

	// Player_UI 캔버스 복원 (이전 라운드에서 DEAD로 인해 꺼졌을 수 있음)
	if (ui_manager) {
		ui_manager->ToggleUI("Player_UI", true, false);
	}
	player_ui_disabled = false;

	if (my_player) {
		my_player->SetCurrentSceneType(SCENE_TYPE::GAME);
		camera->SetTarget(my_player.get());
		if (g_is_single)
			my_player->SetPosition(1.f, 5.0f, 1.5f);	// Lobby 위치 그대로 가져오는 거 방지

		// 다우징 로드가 관리하는 treasuer_position(vector)에 보물 위치 정보를 넣는다.
		auto itemFinder = my_player->GetComponent<CItemFinder>();
		if (itemFinder) {
			itemFinder->RegisterTreasures(treasures);
		}
	}
}

void CGameScene::UpdateMonsters(float elapsedTime)
{
	for (size_t i = 0; i < monster_spawn_info.size(); ) {
		auto& info = monster_spawn_info[i];

		if (!info.monster.expired()) {
			++i;
			continue;
		}

		// 1회성 소환몹은 죽으면 그대로 entry 제거 (리스폰 안 함)
		if (info.is_summoned) {
			std::swap(info, monster_spawn_info.back());
			monster_spawn_info.pop_back();
			continue;
		}

		if (info.respawn_timer < 0.f)
			info.respawn_timer = info.respawn_time;

		info.respawn_timer -= elapsedTime;

		if (info.respawn_timer <= 0.f) {
			auto monster = factory->CreateMonster(info.type, scene_type);
			if (monster) {
				monster->SetPosition(info.position.x, 0.1f, info.position.z);
				monster->SetOriginPos(info.position);
				if (info.type == MON_TYPE::HUMAN_MONSTER) {
					if (auto human = std::dynamic_pointer_cast<CHumanMonster>(monster)) {
						human->SetSpawnCallback([this](MON_TYPE type, XMFLOAT3 pos) {
							auto dog = factory->CreateMonster(type, scene_type);
							if (!dog) return;
							dog->SetPosition(pos.x, pos.y, pos.z);
							dog->SetOriginPos(pos);
							AddObject(dog, dog->GetID());
							monster_spawn_info.push_back({ pos, type, 0.f, -1.f, dog, true });
						});
					}
				}
				AddObject(monster, monster->GetID());
				info.monster = monster;
				info.respawn_time  = monster->GetRespawnTime();
				info.respawn_timer = -1.f;
			}
		}
		++i;
	}
}

void CGameScene::ProcessPickup()
{
	if (my_player->IsIncapacitated())
		return;

	if (CKeyManager::GetInstance().GetKeyState(KEY::Z) == KEY_STATE::TAP) {

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
		CSoundManager::GetInstance().Play(SOUND_ID::pick_up);

		// 싱글환경
		if (g_is_single) {

			auto inv = my_player->GetInventory();
			if (!inv)
				return;

			// 인벤토리 추가에 성공한 경우에만 월드에서 아이템을 제거한다.
			if (inv->AddItem(worldItem->GetItem()))
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
}

void CGameScene::ProcessVisibleObjectMining(float elapsedTime)
{
	// 싱글 전용 
	if (!g_is_single)
		return;

	bool isDigging = (my_player->GetState() == PLAYER_STATE::DIG);

	if (dig_sound_timer >= 0.0f) {
		dig_sound_timer += elapsedTime;
		if (dig_sound_timer >= 0.5f) {
			CSoundManager::GetInstance().Play(SOUND_ID::flying_pan);
			dig_sound_timer = -1.0f;
		}
	}

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
							my_player->SetEquippedItemId(0);
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

				// 다우징로드가 더이상 이 보물을 추적하지 않도록 갱신
				RemoveTreasureFromDowsing(pos);

				pos.y = 0.05f;
				uint16 picked = g_TreasureTable[rand() % g_TreasureTableCount];
				SpawnWorldItem(picked, pos);
			}
		}
		else {
			mining_target = nullptr;
		}
	}

	was_digging = isDigging;

	// 이동 중에는 채굴 시작 불가 
	bool isMoving = KEY_PRESSED(KEY::W) || KEY_PRESSED(KEY::A)
	              || KEY_PRESSED(KEY::S) || KEY_PRESSED(KEY::D);

	// 즉, IDLE 상태이고 좌클릭 눌렀고 (홀딩x), 도구 장착 시에만 채굴 애니메이션 재생
	if (KEY_TAP(KEY::LBTN) && !isDigging && !isMoving) {

		mining_target = nullptr;
		my_player->SetDigAnimFinished(false);

		FindNearestMineTarget(MINEABLEOBJECT_TYPE::VISIBLE);

		if (mining_target)
			dig_sound_timer = 0.0f;

		my_player->SetState(PLAYER_STATE::DIG);
	}
}

void CGameScene::ProcessUnVisibleObjectMining(float elapsedTime)
{
	// 싱글 전용 
	if (!g_is_single)
		return;

	bool isDigging = (my_player->GetState() == PLAYER_STATE::DIG);

	uint16 equippedId = my_player->GetEquippedItemId();
	bool isShovel = equippedId >= 1 && equippedId <= 4;
	bool isBareHand = (equippedId == 0);

	// 이동 중에는 채굴 시작 불가 
	bool isMoving = KEY_PRESSED(KEY::W) || KEY_PRESSED(KEY::A)
		|| KEY_PRESSED(KEY::S) || KEY_PRESSED(KEY::D);

	PlayBareHandDigSound(isBareHand, isMoving);

	if (dig_sound_timer >= 0.0f) {
		dig_sound_timer += elapsedTime;
		if (dig_sound_timer >= 0.5f) {
			if (isShovel) {
				CSoundManager::GetInstance().Play(SOUND_ID::bare_hand_dig, 3.33f);
			}
			dig_sound_timer = -1.0f;
		}
	}

	// DIG→IDLE 전이 감지 = 애니메이션 1회 재생 완료(인터럽트 제외) → 데미지 처리
	if (was_digging && !isDigging && mining_target && !isBareHand && my_player->GetDigAnimFinished()) {

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
							my_player->SetEquippedItemId(0);
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

					size_t idx = std::distance(objects.begin(), it);
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

				// 다우징로드가 더이상 이 보물을 추적하지 않도록 갱신
				RemoveTreasureFromDowsing(pos);

				pos.y = 0.05f;
				uint16 picked = g_TreasureTable[rand() % g_TreasureTableCount];
				SpawnWorldItem(picked, pos);
			}
		}
		else {
			mining_target = nullptr;
		}
	}

	if (isBareHand) {
		if (KEY_PRESSED(KEY::LBTN) && !isMoving && mining_target) {
			bare_hand_dig_timer += elapsedTime;
			if (bare_hand_dig_timer >= 4.0f) {
				bare_hand_dig_timer = 0.0f;

				bool stillExist = false;
				for (auto& obj : objects) {

					if (obj.get() == mining_target) {
						stillExist = true;
						break;
					}
				}

				if (stillExist) {

					mining_target->DestroyImmediate();
					my_player->SetState(PLAYER_STATE::IDLE);
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

							size_t idx = std::distance(objects.begin(), it);
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

						// 다우징로드가 더이상 이 보물을 추적하지 않도록 갱신
						RemoveTreasureFromDowsing(pos);

						pos.y = 0.05f;
						uint16 picked = g_TreasureTable[rand() % g_TreasureTableCount];
						SpawnWorldItem(picked, pos);
					}
				}
			}
		}
		else {
			bare_hand_dig_timer = 0.0f;  // 버튼 뗌 or 이동 시 리셋
		}
	}

	was_digging = isDigging;

	// 즉, IDLE 상태이고 좌클릭 눌렀고 (홀딩x), 도구 장착 시에만 채굴 애니메이션 재생
	if (KEY_TAP(KEY::LBTN) && isShovel && !isDigging && !isMoving) {

		mining_target = nullptr;
		my_player->SetDigAnimFinished(false);

		FindNearestMineTarget(MINEABLEOBJECT_TYPE::NONE_VISIBLE);

		if (mining_target)
			dig_sound_timer = 0.0f;

		my_player->SetState(PLAYER_STATE::DIG);
	}
	else if (KEY_PRESSED(KEY::LBTN) && (equippedId == 0) && !isDigging && !isMoving) {
		mining_target = nullptr;
		my_player->SetDigAnimFinished(false);

		FindNearestMineTarget(MINEABLEOBJECT_TYPE::NONE_VISIBLE);

		if(mining_target)
			my_player->SetState(PLAYER_STATE::DIG);
	}
}

void CGameScene::FindNearestMineTarget(MINEABLEOBJECT_TYPE type)
{
	XMFLOAT3 playerPos = my_player->GetPosition();

	float minDist = 0.0f;
	if (type == MINEABLEOBJECT_TYPE::VISIBLE) {
		minDist = MINING_RANGE;
	}
	else {
		minDist = BARE_HAND_MINING_RANGE;
	}

	for (auto& obj : objects) {

		if (obj->GetObjectType() != OBJECT_TYPE::MINEABLE_OBJECT)
			continue;

		auto mineObj = std::static_pointer_cast<CMineableObject>(obj);
		if (mineObj->GetMineableType() != type)
			continue;

		float dist = Vector3::Length(Vector3::Subtract(obj->GetPosition(), playerPos));
		if (dist < minDist) {
			minDist = dist;
			mining_target = mineObj.get();
		}
	}
}

// 채굴 오브젝트 파괴 시, 다우징로드가 추적하던 보물 정보를 제거하고 갱신한다. (싱글 전용)
void CGameScene::RemoveTreasureFromDowsing(const XMFLOAT3& pos)
{
	// treasure_pos는 ObjectFactory에서 실제 객체 좌표(GetPosition)로 맞춰 두었으므로 위치로 매칭한다.
	auto it = std::find_if(treasures.begin(), treasures.end(),
		[&pos](const TreasureInfo& info) {
			return info.treasure_pos.x == pos.x
				&& info.treasure_pos.y == pos.y
				&& info.treasure_pos.z == pos.z;
		});

	if (it == treasures.end())
		return;

	treasures.erase(it);

	if (my_player) {
		if (auto itemFinder = my_player->GetComponent<CItemFinder>())
			itemFinder->RegisterTreasures(treasures);
	}
}

void CGameScene::ProcessAttack(float elapsedTime)
{
	// 빈사/사망 시 공격 애니메이션·트리거 일체 차단
	if (my_player->IsIncapacitated())
		return;

	auto qs = my_player->GetQuickSlot();
	if (!qs)
		return;

	// 퀵슬롯에서 선택된 아이템의 타입(근접 or 원거리)
	switch (qs->GetSelectedSubType())
	{
	case ITEM_SUB_TYPE::MELEE_WEAPON:
		ProcessMeleeAttack(elapsedTime);
		break;
	case ITEM_SUB_TYPE::RANGED_WEAPON:
		ProcessRangedAttack(elapsedTime);
		break;
	}
}

void CGameScene::ProcessMeleeAttack(float elapsedTime)
{
	if (melee_attack_cooldown > 0.0f)
		melee_attack_cooldown -= elapsedTime;

	// 공격 소리는 서버의 허락을 받지 않고 바로 적용한다.
	if (KEY_TAP(KEY::LBTN) && !g_is_single && !my_player->GetIsKnockedBack() 
		&& !my_player->GetIsPossessed()
		&& !my_player->GetIsStunned()
		&& !my_player->GetDowsing()
		&& my_player->GetState() != PLAYER_STATE::ALMOST_DEAD
		&& my_player->GetState() != PLAYER_STATE::DEAD
		&& melee_attack_cooldown <= 0.0f) {

		melee_attack_cooldown = 1.5f;
		PlayMeleeAttackSound();
		return;
	}

	// 클릭: 애니메이션 시작 + 타이머 세팅
	if (KEY_TAP(KEY::LBTN) && melee_attack_cooldown <= 0.0f
		&& !my_player->GetIsKnockedBack() && !my_player->GetIsPossessed()
		&& !my_player->GetIsStunned()
		&& !my_player->GetDowsing()) {

		my_player->OnAttack();
		melee_attack_timer    = 0.4f;
		melee_attack_cooldown = 1.5f;
		PlayMeleeAttackSound();
	}

	// 타이머 감소 → 만료 시 범위 판정
	if (melee_attack_timer > 0.0f) {
		melee_attack_timer -= elapsedTime;
		if (melee_attack_timer <= 0.0f) {
			melee_attack_timer = -1.0f;

			XMFLOAT3 playerPos = my_player->GetPosition();
			XMFLOAT3 look      = my_player->look;
			look.y = 0.0f;
			float lookLen = sqrtf(look.x * look.x + look.z * look.z);
			if (lookLen >= 0.001f) {
				look.x /= lookLen;
				look.z /= lookLen;

				XMFLOAT3 right = { look.z, 0.0f, -look.x };

				constexpr float MELEE_DEPTH      = 1.1f;
				constexpr float MELEE_HALF_WIDTH = 0.6f;

				for (auto& obj : objects) {
					if (obj->GetObjectType() != OBJECT_TYPE::MONSTER) 
						continue;

					auto* monster = static_cast<CMonster*>(obj.get());
					MON_TYPE mType = monster->GetMonsterType();
					if (mType != MON_TYPE::HUMAN_MONSTER && mType != MON_TYPE::ANIMAL_MONSTER) 
						continue;

					XMFLOAT3 toMonster = Vector3::Subtract(monster->GetPosition(), playerPos);
					toMonster.y = 0.0f;

					float forwardDist = look.x * toMonster.x + look.z * toMonster.z;
					if (forwardDist < 0.0f || forwardDist > MELEE_DEPTH) 
						continue;

					float lateralDist = fabsf(right.x * toMonster.x + right.z * toMonster.z);
					if (lateralDist > MELEE_HALF_WIDTH) 
						continue;

					monster->ApplyMeleeHit(playerPos);
				}
			}
		}
	}
}

void CGameScene::ProcessRangedAttack(float elapsedTime)
{
	if (!g_is_single)
		return;

	uint16 equippedID = my_player->GetEquippedItemId();
	auto inv = my_player->GetInventory();
	auto qs = my_player->GetQuickSlot();
	const auto& items = inv->GetItems();
	auto it = items.find(qs->GetSelectedInvId());

	switch (equippedID)
	{
		case 16: // 스프레이
		{
			auto spray = std::static_pointer_cast<CWeapon>(it->second);

			if (KEY_TAP(KEY::LBTN) && spray_attack_cooldown <= 0.0f
				&& !my_player->GetIsKnockedBack() && !my_player->GetIsPossessed()
				&& !my_player->GetIsStunned()
				&& !my_player->GetDowsing()) {
				my_player->OnAttack();
				spray->ReduceDurability();
				CSoundManager::GetInstance().Play(SOUND_ID::ghost_spray);
				spray_attack_timer = 0.8f;
				spray_attack_cooldown = 1.6f;
			}

			// 0.8초 후 데미지 적용
			if (spray_attack_timer > 0.0f) {
				spray_attack_timer -= elapsedTime;
				if (spray_attack_timer <= 0.0f) {
					spray_attack_timer = -1.0f;
					SprayAttack(elapsedTime);
					if (spray->GetCurrentDurability() <= 0) {
						inv->RemoveItem(qs->GetSelectedInvId());
						my_player->SetEquippedItemId(0);
					}
				}
			}

			if (spray_attack_cooldown > 0.0f)
				spray_attack_cooldown -= elapsedTime;
		}
		break;
		case 17: // 마법 지팡이
		{
			if (KEY_TAP(KEY::LBTN) && !my_player->GetIsKnockedBack()
				&& !my_player->GetIsPossessed()
				&& !my_player->GetIsStunned()
				&& !my_player->GetDowsing()) {
				my_player->OnAttack();
			}
		}
		break;
		case 18: // 비비탄총
		{
			if (KEY_TAP(KEY::LBTN) && !my_player->GetIsKnockedBack()
				&& !my_player->GetIsPossessed()
				&& !my_player->GetIsStunned()
				&& !my_player->GetDowsing()) {
				my_player->OnAttack();
			}
		}
		break;
	}
}

void CGameScene::SprayAttack(float elapsedTime)
{
	constexpr float SPRAY_RANGE = 1.5f;

	XMFLOAT3 playerPos  = my_player->GetPosition();
	XMFLOAT3 playerLook = my_player->look;

	for (auto& obj : objects) {
		if (obj->GetObjectType() != OBJECT_TYPE::MONSTER) 
			continue;

		auto* monster = static_cast<CMonster*>(obj.get());
		if (monster->GetMonsterType() != MON_TYPE::GHOST) 
			continue;
		if (monster->GetAIState() == AI_STATE::MONSTER_FLEE) 
			continue;

		XMFLOAT3 toMonster = Vector3::Subtract(monster->GetPosition(), playerPos);
		toMonster.y = 0.0f;
		float dist = Vector3::Length(toMonster);

		if (dist > SPRAY_RANGE || dist < 0.001f) 
			continue;

		// 전방 체크: 플레이어 정면 방향에 있는지
		float dot = playerLook.x * (toMonster.x / dist) + playerLook.z * (toMonster.z / dist);
		if (dot <= 0.0f) 
			continue;

		static_cast<CGhost*>(monster)->ApplySprayHit(playerPos);
	}
}

// 싱글환경
void CGameScene::SpawnWorldItem(uint16 itemID, XMFLOAT3 position, int16 dur)
{
	auto worldItem = factory->CreateWorldItem(itemID);
	if (!worldItem)
		return;

	worldItem->Initialize();

	// 아이템의 위치
	worldItem->SetPosition(position);

	// 아이템의 ID (CObject 클래스에 정의된 obj_id)
	worldItem->SetID(world_item_id_counter);

	// 내구도 복원 (드롭으로 떨어진 장비의 내구도 보존)
	if (dur > 0) {
		auto equipment = std::static_pointer_cast<CEquipment>(worldItem->GetItem());
		equipment->SetCurrentDurability((uint16)dur);
	}

	// Scene의 objects 컨테이너에 추가
	AddObject(worldItem, world_item_id_counter);

	++world_item_id_counter;
}

// 멀티환경
void CGameScene::SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position, int16 dur)
{
	auto worldItem = factory->CreateWorldItem(itemID);
	if (!worldItem)
		return;

	worldItem->Initialize();

	// 아이템의 위치
	worldItem->SetPosition(position);

	// 아이템의 ID (CObject 클래스에 정의된 obj_id)
	worldItem->SetID(itemWorldId);

	if (dur > 0) {
		auto equipment = std::static_pointer_cast<CEquipment>(worldItem->GetItem());
		equipment->SetCurrentDurability((uint16)dur);
	}

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
	pos.y           = std::max(pos.y, 0.0f);
	uint32   worldId = world_item_id_counter; // SpawnWorldItem 호출 전에 캡처

	// 장비면 현재 내구도를 보존해서 넘긴다 (재픽업 시 내구도 복구 버그 방지)
	int16 dur = -1;
	if (item->GetItemType() == ITEM_TYPE::EQUIPMENT) {
		auto equipment = std::static_pointer_cast<CEquipment>(item);
		dur = (int16)equipment->GetCurrentDurability();
	}

	SpawnWorldItem(item->GetItemId(), pos, dur);  // 내부에서 world_item_id_counter 증가
}

void CGameScene::UpdateCHoldAction(float elapsedTime)
{
	CHOLD_TARGET targetType = CHOLD_TARGET::NONE;

	if (KEY_PRESSED(KEY::C)) {
		XMFLOAT3 myPos = my_player->GetPosition();

		for (auto& obj : objects) {
			auto* player = dynamic_cast<CPlayer*>(obj.get());

			if (!player || player == my_player.get())
				continue;

			bool isPossessed = player->GetIsPossessed();
			bool isRescuable = (player->GetState() == PLAYER_STATE::ALMOST_DEAD);
			if (!isPossessed && !isRescuable)
				continue;

			XMFLOAT3 diff = Vector3::Subtract(player->GetPosition(), myPos);
			diff.y = 0.0f;
			if (Vector3::Length(diff) <= 0.9f) {
				targetType = isPossessed ? CHOLD_TARGET::POSSESSION : CHOLD_TARGET::RESCUE;
				break;
			}
		}
	}

	if (targetType != CHOLD_TARGET::NONE) {
		// 대상 종류가 바뀌면 타이머 리셋 (UI 진행률 점프 방지)
		if (my_player->GetCHoldTarget() != targetType) {
			my_player->ResetCHoldTimer();
			my_player->SetCHoldTarget(targetType);
		}
		my_player->UpdateCHoldTimer(elapsedTime);
	}
	else {
		CSoundManager::GetInstance().Stop(SOUND_ID::clock_alarm);
		my_player->ResetCHoldTimer();
		my_player->SetCHoldTarget(CHOLD_TARGET::NONE);
	}
}

void CGameScene::DrawDePossessProgressBar()
{
	ImVec2 sz = ImGui::GetIO().DisplaySize;

	// BASE_UI_HEIGHT(600) 기준, G_RATIO_Y로 해상도 대응
	const float iconSize  = 150.f * G_RATIO_Y;
	const float barWidth  = 407.f * G_RATIO_Y;
	const float barHeight = 38.f  * G_RATIO_Y;

	// 바가 유령 이미지 중간부터 시작 → 유령이 바를 덮는 겹침 효과
	const float overlapX  = iconSize * 0.55f;
	const float winWidth  = overlapX + barWidth;
	const float winHeight = iconSize;

	ImGui::SetNextWindowPos(ImVec2(sz.x * 0.5f - winWidth * 0.5f - 50.f, sz.y - 268.f * G_RATIO_Y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winWidth, winHeight));
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

	ImGui::Begin("##possession_release", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	// 1. 진행 바를 먼저 그림 (유령 뒤에 렌더링)
	float barY = (winHeight - barHeight) * 0.5f;
	ImGui::SetCursorPos(ImVec2(overlapX, barY));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.33f, 0.47f, 0.82f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.33f, 0.47f, 0.82f, 0.4f));
	ImGui::ProgressBar(my_player->GetCHoldProgress(), ImVec2(barWidth, barHeight), "");
	ImGui::PopStyleColor(2);

	// 2. 유령 아이콘을 위에 그림 (진행 바를 덮음)
	ImGui::SetCursorPos(ImVec2(0.f, 0.f));
	ImTextureID ghostTex = CImGuiManager::GetInstance().GetTexture("ghost_icon");
	ImGui::Image(ghostTex, ImVec2(iconSize, iconSize));

	ImGui::End();
	ImGui::PopStyleVar(3);
}

void CGameScene::DrawRescueProgressBar()
{
	// 빈사 소생 진행바. 빙의 해제 바와 구분하기 위해 초록색 + heal 아이콘 사용.
	ImVec2 sz = ImGui::GetIO().DisplaySize;

	// BASE_UI_HEIGHT(600) 기준, G_RATIO_Y로 해상도 대응
	const float iconSize  = 120.f * G_RATIO_Y;
	const float barWidth  = 407.f * G_RATIO_Y;
	const float barHeight = 38.f  * G_RATIO_Y;

	// 바가 아이콘 중간부터 시작 → 아이콘이 바를 덮는 겹침 효과
	const float overlapX  = iconSize * 0.55f;
	const float winWidth  = overlapX + barWidth;
	const float winHeight = iconSize;

	ImGui::SetNextWindowPos(ImVec2(sz.x * 0.5f - winWidth * 0.5f - 50.f, sz.y - 253.f * G_RATIO_Y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winWidth, winHeight));
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

	ImGui::Begin("##rescue_progress", nullptr,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse);

	// 1. 진행 바를 먼저 그림 (아이콘 뒤에 렌더링)
	float barY = (winHeight - barHeight) * 0.5f;
	ImGui::SetCursorPos(ImVec2(overlapX, barY));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.85f, 0.3f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg,       ImVec4(0.3f, 0.85f, 0.3f, 0.4f));
	ImGui::ProgressBar(my_player->GetCHoldProgress(), ImVec2(barWidth, barHeight), "");
	ImGui::PopStyleColor(2);

	// 2. heal 아이콘을 위에 그림 (진행 바를 덮음)
	ImGui::SetCursorPos(ImVec2(0.f, 0.f));
	ImTextureID healTex = CImGuiManager::GetInstance().GetTexture("heal_icon");
	ImGui::Image(healTex, ImVec2(iconSize, iconSize));

	ImGui::End();
	ImGui::PopStyleVar(3);
}

void CGameScene::PlayMeleeAttackSound()
{
	uint16 equippedID = my_player->GetEquippedItemId();
	if (equippedID == 14) {
		CSoundManager::GetInstance().Play(SOUND_ID::swing2);
	}
	else if (equippedID == 19) {
		CSoundManager::GetInstance().Play(SOUND_ID::sword);
	}
}

void CGameScene::PlayBareHandDigSound(bool isBareHand, bool isMoving)
{
	bool bareHandDigging = isBareHand && KEY_PRESSED(KEY::LBTN) && !isMoving && mining_target;
	if (bareHandDigging && !bare_hand_dig_loop_playing) {
		CSoundManager::GetInstance().Play(SOUND_ID::bare_hand_dig, 0, 3.33f); // 0 = 무한 반복
		bare_hand_dig_loop_playing = true;
	}
	else if (!bareHandDigging && bare_hand_dig_loop_playing) {
		CSoundManager::GetInstance().Stop(SOUND_ID::bare_hand_dig);
		bare_hand_dig_loop_playing = false;
	}
}

void CGameScene::TriggerSinglePlayerSettlement()
{
	if (show_settlement_modal || !my_player)
		return;

	settlement_result = SettlementResult{};
	settlement_result.is_returned = my_player->GetReturned();
	// 싱글 1인 = 복귀했으면 전원 복귀 보너스 적용
	settlement_result.all_returned_bonus = my_player->GetReturned();

	auto inv = my_player->GetInventory();
	if (inv) {
		// item_id별 묶음
		std::unordered_map<uint16, SettlementEntry> grouped;
		for (auto& [invId, item] : inv->GetItems()) {
			if (item->GetItemType() != ITEM_TYPE::TREASURE)
				continue;

			auto treasure = std::dynamic_pointer_cast<CTreasure>(item);
			if (!treasure)
				continue;

			uint16 iid = static_cast<uint16>(treasure->GetItemId());
			if (grouped.find(iid) == grouped.end()) {
				SettlementEntry e;
				e.item_id   = iid;
				e.price     = treasure->GetPrice();
				e.count     = 0;
				e.name      = treasure->GetName();
				e.icon_path = treasure->GetIconPath();
				grouped[iid] = std::move(e);
			}
			grouped[iid].count += 1;
			settlement_result.base_coin += treasure->GetPrice();
		}
		for (auto& [iid, e] : grouped)
			settlement_result.entries.push_back(e);

		float rate  = settlement_result.is_returned ? 1.0f : 0.5f;
		float bonus = settlement_result.all_returned_bonus ? 2.0f : 1.0f;
		settlement_result.final_coin = static_cast<uint32>(settlement_result.base_coin * rate * bonus);

		my_player->SetGold(my_player->GetGold() + settlement_result.final_coin);

		// 보물 인벤토리에서 제거
		std::vector<uint32> to_remove;
		for (auto& [invId, item] : inv->GetItems()) {
			if (item->GetItemType() == ITEM_TYPE::TREASURE)
				to_remove.push_back(invId);
		}
		for (auto id : to_remove)
			inv->RemoveItem(id);
	}

	show_settlement_modal = true;

	// 정산 완료 후 복귀 상태로 전환 → 몬스터 타겟에서 제외
	if (my_player && !my_player->GetReturned())
		my_player->SetReturned(true);
}

// 싱글 전용
void CGameScene::DetectMyPlayerReturn()
{
	if (!return_active || my_player->GetReturned() || !my_player)
		return;

	const XMFLOAT3& pp = my_player->GetPosition();
	float dx = pp.x - return_center.x;
	float dz = pp.z - return_center.z;
	if (dx * dx + dz * dz > return_range * return_range)
		return;

	my_player->SetReturned(true);
	my_player->SetState(PLAYER_STATE::IDLE);
	my_player->SetVelocity(0.f, 0.f, 0.f);
	CSoundManager::GetInstance().Play(SOUND_ID::Return);

	ReturnToast toast;
	toast.is_self = true;
	toast.timer   = RETURN_TOAST_DURATION;
	// "복귀 완료"
	toast.text    = "\xEB\xB3\xB5\xEA\xB7\x80 \xEC\x99\x84\xEB\xA3\x8C";
	return_toasts.push_back(std::move(toast));
}

void CGameScene::DrawReturnToasts()
{
	if (return_toasts.empty())
		return;

	ImGuiIO& io = ImGui::GetIO();

	// 타이머 감소 + 만료 제거 (ImGui 자체 DeltaTime 사용)
	for (auto& t : return_toasts) t.timer -= io.DeltaTime;
	return_toasts.erase(
		std::remove_if(return_toasts.begin(), return_toasts.end(),
			[](const ReturnToast& t) { return t.timer <= 0.f; }),
		return_toasts.end());

	if (return_toasts.empty())
		return;

	ImDrawList* dl   = ImGui::GetForegroundDrawList();
	ImFont*     font = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();
	const float scale = G_RATIO_Y;

	// 라운드 타이머 아래에서 시작
	float yCursor = 80.f * scale;

	for (const auto& toast : return_toasts) {
		float fontPx = toast.is_self ? 48.f * scale : 24.f * scale;

		// 마지막 0.5초 페이드 아웃
		float alpha = (toast.timer < 0.5f) ? (toast.timer / 0.5f) : 1.f;

		ImVec2 textSize = font->CalcTextSizeA(fontPx, FLT_MAX, 0.f, toast.text.c_str());
		ImVec2 pos      = ImVec2(io.DisplaySize.x * 0.5f - textSize.x * 0.5f, yCursor);

		ImVec4 color  = toast.is_self ? ImVec4(1.f, 0.85f, 0.25f, alpha)
		                              : ImVec4(0.9f, 0.9f, 0.9f, alpha);
		ImU32  shadow = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.7f * alpha));
		const float off = 1.5f * scale;

		dl->AddText(font, fontPx, ImVec2(pos.x - off, pos.y), shadow, toast.text.c_str());
		dl->AddText(font, fontPx, ImVec2(pos.x + off, pos.y), shadow, toast.text.c_str());
		dl->AddText(font, fontPx, ImVec2(pos.x, pos.y - off), shadow, toast.text.c_str());
		dl->AddText(font, fontPx, ImVec2(pos.x, pos.y + off), shadow, toast.text.c_str());
		dl->AddText(font, fontPx, pos, ImGui::GetColorU32(color), toast.text.c_str());

		yCursor += textSize.y + 8.f * scale;
	}
}

void CGameScene::DrawReturnMarker()
{
	constexpr int   SEGMENTS = 48;
	constexpr float TWO_PI   = 6.28318530718f;

	ImGuiIO&    io = ImGui::GetIO();
	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// 펄스 효과 (0.6 ~ 1.0 알파, 2Hz)
	float t     = (float)ImGui::GetTime();
	float pulse = 0.5f + 0.5f * sinf(t * 2.0f);
	float alpha = 0.6f + 0.4f * pulse;

	XMFLOAT4X4 view = camera->GetViewMatrix();
	XMFLOAT4X4 proj = camera->GetProjectionMatrix();
	XMMATRIX   vp   = XMMatrixMultiply(XMLoadFloat4x4(&view), XMLoadFloat4x4(&proj));

	ImVec2 pts[SEGMENTS];
	bool   valid[SEGMENTS];

	for (int i = 0; i < SEGMENTS; ++i) {
		float    theta = (float)i / (float)SEGMENTS * TWO_PI;
		XMVECTOR wp    = XMVectorSet(return_center.x + cosf(theta) * return_range,
		                             return_center.y + 0.05f,   // z-fight 방지
		                             return_center.z + sinf(theta) * return_range,
		                             1.f);
		XMVECTOR clip  = XMVector4Transform(wp, vp);
		float    cw    = XMVectorGetW(clip);

		if (cw <= 0.01f) {
			valid[i] = false;
			pts[i]   = {};
			continue;
		}
		float ndcX = XMVectorGetX(clip) / cw;
		float ndcY = XMVectorGetY(clip) / cw;
		pts[i]   = ImVec2((ndcX * 0.5f + 0.5f) * io.DisplaySize.x,
		                  (1.f - (ndcY * 0.5f + 0.5f)) * io.DisplaySize.y);
		valid[i] = true;
	}

	// 노란 링 (두께 3px, 카메라 뒤 정점 구간은 스킵)
	ImU32 color     = ImGui::GetColorU32(ImVec4(1.0f, 0.85f, 0.25f, alpha));
	float thickness = 3.0f * G_RATIO_Y;
	for (int i = 0; i < SEGMENTS; ++i) {
		int j = (i + 1) % SEGMENTS;
		if (valid[i] && valid[j])
			dl->AddLine(pts[i], pts[j], color, thickness);
	}
}

void CGameScene::DrawSettlementModal()
{
	if (!show_settlement_modal)
		return;

	ImGuiIO& io = ImGui::GetIO();
	const float scale  = G_RATIO_Y;
	const float wW     = io.DisplaySize.x * 0.60f;
	const float wH     = io.DisplaySize.y * 0.60f;
	const float tableH = wH * 0.54f;

	// 매 프레임 OpenPopup (BeginPopupModal 전에 호출해야 처음에 열림)
	ImGui::OpenPopup("\xEC\xA0\x95\xEC\x82\xB0 \xEA\xB2\xB0\xEA\xB3\xBC");  // "정산 결과"

	ImGui::SetNextWindowSize(ImVec2(wW, wH), ImGuiCond_Always);
	ImGui::SetNextWindowPos(
		ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
		ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	// === 다크 골드(보물) 테마: 컬러/변수 푸시 ===
	ImGui::PushStyleColor(ImGuiCol_PopupBg,          ImVec4(0.10f, 0.10f, 0.13f, 0.97f));
	ImGui::PushStyleColor(ImGuiCol_TitleBg,          ImVec4(0.22f, 0.16f, 0.08f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive,    ImVec4(0.34f, 0.24f, 0.10f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_Border,           ImVec4(0.72f, 0.56f, 0.24f, 0.90f));
	ImGui::PushStyleColor(ImGuiCol_TableHeaderBg,    ImVec4(0.23f, 0.18f, 0.10f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_TableRowBg,       ImVec4(0.13f, 0.12f, 0.14f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt,    ImVec4(0.17f, 0.15f, 0.17f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_TableBorderLight, ImVec4(0.45f, 0.38f, 0.22f, 0.55f));
	ImGui::PushStyleColor(ImGuiCol_Separator,        ImVec4(0.72f, 0.56f, 0.24f, 0.75f));
	ImGui::PushStyleColor(ImGuiCol_Button,           ImVec4(0.45f, 0.32f, 0.12f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered,    ImVec4(0.72f, 0.50f, 0.18f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,     ImVec4(0.38f, 0.26f, 0.08f, 1.00f));
	ImGui::PushStyleColor(ImGuiCol_Text,             ImVec4(0.95f, 0.92f, 0.85f, 1.00f));

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   8.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,    5.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,  1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(14.f * scale, 10.f * scale));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,      ImVec2(8.f * scale, 5.f * scale));

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
	                       | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	if (!ImGui::BeginPopupModal("\xEC\xA0\x95\xEC\x82\xB0 \xEA\xB2\xB0\xEA\xB3\xBC", nullptr, flags)) {
		ImGui::PopStyleVar(6);
		ImGui::PopStyleColor(13);
		return;
	}

	// enlarge font for readability (proportional to UI scale)
	ImGui::SetWindowFontScale(scale * 1.1f);

	// helper: draw text right-aligned within the content region
	char fmtBuf[128];
	auto rightAlignedText = [](const char* text) {
		float textWidth = ImGui::CalcTextSize(text).x;
		float avail = ImGui::GetContentRegionAvail().x;
		if (avail > textWidth)
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - textWidth);
		ImGui::TextUnformatted(text);
	};

	ImFont* boldFont = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();
	const float rowH  = 35.f * scale;
	const float iconSz = 32.f * scale;

	// 보물 목록 테이블
	ImGui::PushFont(boldFont);
	if (!settlement_result.entries.empty()) {
		// 보물 개수만큼 테이블 높이 동적 계산 (헤더 + 보이는 행, 최대 tableH로 캡)
		const float headerRowH = ImGui::GetTextLineHeight() + 8.f * scale;
		const int   visibleN   = (int)std::min<size_t>(settlement_result.entries.size(), 6);
		const float wantTableH = headerRowH + visibleN * rowH + 6.f * scale;

		// 보물이 많아도 하단(합계 + 최종 코인 박스 + 로비 복귀 버튼)이 잘리지 않도록
		// 테이블 높이를 "현재 남은 영역 - 푸터 예약치" 이내로 제한한다. (초과분은 테이블 내부 스크롤)
		const float footerReserve = ImGui::GetTextLineHeightWithSpacing() * 2.0f       // 보물 합계 + 보너스 줄
		                          + ImGui::GetTextLineHeight() * 1.45f + 8.f * scale    // 최종 코인 박스
		                          + (30.f * scale + 12.f * scale)                       // 버튼 + 하단 패딩
		                          + 28.f * scale;                                       // 구분선 2개 + 간격 여유
		const float maxTableH  = std::max(rowH, ImGui::GetContentRegionAvail().y - footerReserve);
		const float useTableH  = std::min(std::min(tableH, wantTableH), maxTableH);

		ImGuiTableFlags tblFlags = ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
		if (ImGui::BeginTable("##settle_tbl", 4, tblFlags, ImVec2(ImGui::GetContentRegionAvail().x, useTableH))) {
			ImGui::TableSetupScrollFreeze(0, 1);
			ImGui::TableSetupColumn("",    ImGuiTableColumnFlags_WidthFixed, iconSz + 20.f * scale);
			ImGui::TableSetupColumn("\xEB\xB3\xB4\xEB\xAC\xBC \xEC\x9D\xB4\xEB\xA6\x84",  // "보물 이름"
			    ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("\xEA\xB0\x9C\xEB\x8B\xB9 \xEA\xB0\x80\xEA\xB2\xA9", // "개당 가격"
			    ImGuiTableColumnFlags_WidthFixed, 140.f * G_RATIO_X);
			ImGui::TableSetupColumn("\xEA\xB0\x9C\xEC\x88\x98",  // "개수"
			    ImGuiTableColumnFlags_WidthFixed, 85.f * G_RATIO_X);
			ImGui::TableHeadersRow();

			for (const auto& e : settlement_result.entries) {
				ImGui::TableNextRow(0, rowH);
				ImGui::SetWindowFontScale(scale * 0.9f);

				// 아이콘 (어두운 라운드 슬롯 + 금테)
				ImGui::TableSetColumnIndex(0);
				{
					ImVec2 cursor = ImGui::GetCursorScreenPos();
					ImDrawList* dl = ImGui::GetWindowDrawList();

					ImVec2 iconMin(cursor.x + 4.f * scale, cursor.y + 4.f * scale);
					ImVec2 iconMax(cursor.x + iconSz + 4.f * scale, cursor.y + iconSz + 4.f * scale);
					ImVec2 slotMin(iconMin.x - 2.f * scale, iconMin.y - 2.f * scale);
					ImVec2 slotMax(iconMax.x + 2.f * scale, iconMax.y + 2.f * scale);

					dl->AddRectFilled(slotMin, slotMax, IM_COL32(210, 210, 215, 255), 4.f * scale);
					dl->AddRect      (slotMin, slotMax, IM_COL32(190, 150, 70, 220), 4.f * scale, 0, 1.f);

					ImTextureID tex = (ImTextureID)nullptr;
					if (!e.icon_path.empty())
						tex = CImGuiManager::GetInstance().GetTexture(e.icon_path);
					if (tex)
						dl->AddImage(tex, iconMin, iconMax);
				}
				ImGui::Dummy(ImVec2(iconSz + 8.f * scale, rowH));

				// 이름
				ImGui::TableSetColumnIndex(1);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (rowH - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::TextUnformatted(e.name.c_str());

				// 개당 가격 (골드 톤)
				ImGui::TableSetColumnIndex(2);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (rowH - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.85f, 0.45f, 1.f));
				ImGui::Text("%u", e.price);
				ImGui::PopStyleColor();

				// 개수
				ImGui::TableSetColumnIndex(3);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (rowH - ImGui::GetTextLineHeight()) * 0.5f);
				ImGui::Text("x%u", e.count);
			}
			ImGui::EndTable();
		}
	}
	else {
		// "획득한 보물 없음"
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.68f, 0.62f, 1.f));
		ImGui::TextUnformatted("\xED\x9A\x8D\xEB\x93\x9D\xED\x95\x9C \xEB\xB3\xB4\xEB\xAC\xBC \xEC\x97\x86\xEC\x9D\x8C");
		ImGui::PopStyleColor();
	}
	ImGui::PopFont();

	ImGui::Separator();

	// 보물 합계
	ImGui::PushFont(boldFont);
	// "보물 합계: %u 코인"
	snprintf(fmtBuf, sizeof(fmtBuf), "\xEB\xB3\xB4\xEB\xAC\xBC \xED\x95\xA9\xEA\xB3\x84: %u \xEC\xBD\x94\xEC\x9D\xB8",
	    settlement_result.base_coin);
	rightAlignedText(fmtBuf);
	ImGui::PopFont();

	// 보너스/배율
	if (settlement_result.all_returned_bonus) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.85f, 1.00f, 1.f));
		// "복귀 보너스 x2!"
		rightAlignedText("\xEB\xB3\xB4\xEB\x84\x88\xEC\x8A\xA4 x2!");
		ImGui::PopStyleColor();
	}
	else if (!settlement_result.is_returned) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.45f, 0.45f, 1.f));
		// "미복귀 x0.5"
		rightAlignedText("\xEB\xAF\xB8\xEB\xB3\xB5\xEA\xB7\x80 x0.5");
		ImGui::PopStyleColor();
	}

	ImGui::Separator();

	// === 최종 코인: 골드 하이라이트 박스 ===
	ImGui::PushFont(boldFont);
	{
		// "최종 코인: %u"
		snprintf(fmtBuf, sizeof(fmtBuf), "\xEC\xB5\x9C\xEC\xA2\x85 \xEC\xBD\x94\xEC\x9D\xB8: %u", settlement_result.final_coin);

		ImVec2 boxStart  = ImGui::GetCursorScreenPos();
		float  boxLocalY = ImGui::GetCursorPosY();
		float  boxW      = ImGui::GetContentRegionAvail().x;
		float  boxH      = ImGui::GetTextLineHeight() * 1.45f;

		ImDrawList* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(boxStart, ImVec2(boxStart.x + boxW, boxStart.y + boxH),
		    IM_COL32(45, 35, 18, 240), 5.f * scale);
		dl->AddRect      (boxStart, ImVec2(boxStart.x + boxW, boxStart.y + boxH),
		    IM_COL32(225, 180, 80, 255), 5.f * scale, 0, 1.5f);

		float textW    = ImGui::CalcTextSize(fmtBuf).x;
		float textH    = ImGui::GetTextLineHeight();
		float rightPad = 14.f * scale;
		ImGui::SetCursorPosY(boxLocalY + (boxH - textH) * 0.5f);
		float curX = ImGui::GetCursorPosX();
		if (boxW > textW + rightPad)
			ImGui::SetCursorPosX(curX + boxW - textW - rightPad);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.85f, 0.30f, 1.f));
		ImGui::TextUnformatted(fmtBuf);
		ImGui::PopStyleColor();

		// 박스 하단으로 커서 이동
		ImGui::SetCursorPosY(boxLocalY + boxH + 2.f * scale);
	}
	ImGui::PopFont();

	// "로비로 복귀" 버튼 - 하단 고정
	float btnW = 130.f * G_RATIO_X;
	float btnH = 30.f * scale;
	float pad  = 12.f * scale;
	float remainY = ImGui::GetContentRegionAvail().y;
	if (remainY > btnH + pad)
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + remainY - btnH - pad);
	ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - btnW) * 0.5f + ImGui::GetCursorPosX());
	if (ImGui::Button("\xEB\xA1\x9C\xEB\xB9\x84\xEB\xA1\x9C \xEB\xB3\xB5\xEA\xB7\x80",  // "로비로 복귀"
	    ImVec2(btnW, btnH))) {
		show_settlement_modal = false;
		PlayClickSound();
		ImGui::CloseCurrentPopup();

		// 게임 종료 → 다음 로비 진입 시 상점 재고/랜덤/가격 초기화
		CShop::GetInstance().Reset();

		if (g_is_single) {
			CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);
		}
		else {
			C_SceneChange pkt;
			pkt.player_id     = my_player->GetID();
			pkt.current_scene = SCENE_TYPE::GAME;
			pkt.target_scene  = SCENE_TYPE::LOBBY;
			auto sendBuffer = CServerPacketHandler::MakeSendBuffer(pkt);
			my_player->GetSession()->DoSend(sendBuffer);
		}
	}
	CheckHoverSound();

	ImGui::EndPopup();

	ImGui::PopStyleVar(6);
	ImGui::PopStyleColor(13);
}

void CGameScene::DrawOpponentStatus()
{
	// 상대가 없으면(싱글/대기) 아무것도 그리지 않음
	if (player_slot_ids.empty())
		return;

	ImGuiIO& io = ImGui::GetIO();
	const float sx = G_RATIO_X;
	const float sy = G_RATIO_Y;

	// BackgroundDrawList: 3D 씬 위 + 모든 ImGui 윈도우(인벤토리 등) 아래에 그림
	// → 인벤토리를 열면 상대 UI가 그 아래로 덮인다
	ImDrawList* dl = ImGui::GetBackgroundDrawList();

	// 800x600 기준 레이아웃 (×G_RATIO)
	const float radius         = 24.f * sy;   // 원 반지름
	const float rightMargin    = 12.f * sx;   // 우측 여백
	const float topStart       = 25.f * sy;   // 첫 행 시작 Y (라운드 타이머 아래)
	const float rowGap         = 78.f * sy;   // 행 간 세로 간격
	const float gapCircleToBar = 2.5f * sx;   // 원-HP바 간격
	const float barW           = 150.f * sx;  // HP바 너비
	const float barH           = 16.f * sy;   // HP바 높이

	int drawn = 0;
	for (uint64 id : player_slot_ids) {
		if (drawn >= 3) break;                 // 표시 가능한 상대는 최대 3명

		auto it = id_To_Index.find(id);
		if (it == id_To_Index.end()) continue;

		// player_slot_ids는 플레이어 ID만 보유 → CPlayer 확정. null 슬롯 방어만 유지
		auto player = std::static_pointer_cast<CPlayer>(objects[it->second]);
		if (!player) 
			continue;

		// 행 중심 Y
		const float rowCenterY = topStart + radius + rowGap * drawn;

		// 전체(원+HP바)를 우측 정렬 → 원 중심 X 계산
		const float circleCenterX = io.DisplaySize.x - rightMargin - barW - gapCircleToBar - radius;
		const ImVec2 circleCenter = ImVec2(circleCenterX, rowCenterY);

		// ── 원 안 사진 (완전한 원형 클리핑: 정사각형에 rounding=radius) ──
		ImTextureID tex = 0;
		const char* key = CResourceManager::GetInstance().PlayerImageKey(player->GetPlayerImage());
		if (key) 
			tex = CImGuiManager::GetInstance().GetTexture(key);

		const ImVec2 imgMin = ImVec2(circleCenter.x - radius, circleCenter.y - radius);
		const ImVec2 imgMax = ImVec2(circleCenter.x + radius, circleCenter.y + radius);

		if (tex) {
			dl->AddImageRounded(tex, imgMin, imgMax,
				ImVec2(0, 0), ImVec2(1, 1),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 1)),
				radius, ImDrawFlags_RoundCornersAll);
		}
		else {
			// 텍스처 없을 때 플레이스홀더 (회색 원)
			dl->AddCircleFilled(circleCenter, radius,
				ImGui::GetColorU32(ImVec4(0.30f, 0.30f, 0.35f, 1.f)), 32);
		}

		// 사망 시 사진(원)을 어둡게 처리 — 죽은 상태 강조
		if (player->GetState() == PLAYER_STATE::DEAD) {
			dl->AddCircleFilled(circleCenter, radius,
				ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.55f)), 32);
		}

		// 원 테두리 링
		dl->AddCircle(circleCenter, radius,
			ImGui::GetColorU32(ImVec4(0.20f, 0.35f, 0.85f, 1.f)), 32, 2.5f * sy);

		// ── HP바 ──
		const float hpMax   = static_cast<float>(player->GetMaxHp());
		float       hpRatio = (hpMax > 0.f) ? static_cast<float>(player->GetHp()) / hpMax : 0.f;
		if (hpRatio < 0.f) hpRatio = 0.f;
		if (hpRatio > 1.f) hpRatio = 1.f;

		const float barX = circleCenter.x + radius + gapCircleToBar;
		const float barY = rowCenterY - barH * 0.5f + (10 * sy);
		const ImVec2 barMin  = ImVec2(barX, barY);
		const ImVec2 barMax  = ImVec2(barX + barW, barY + barH);
		const ImVec2 fillMax = ImVec2(barX + barW * hpRatio, barY + barH);

		// 배경 → 빨강 채움 → 테두리
		dl->AddRectFilled(barMin, barMax,
			ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.12f, 0.85f)), 3.f * sy);
		dl->AddRectFilled(barMin, fillMax,
			ImGui::GetColorU32(ImVec4(0.85f, 0.15f, 0.15f, 1.f)), 3.f * sy);
		dl->AddRect(barMin, barMax,
			ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.9f)), 3.f * sy, 0, 1.5f * sy);

		// ── 상태 아이콘 (HP바 바로 위, 우선순위: 사망 > 빈사 > 빙의 / 정상이면 없음) ──
		const char* statusKey = nullptr;
		if      (player->GetState() == PLAYER_STATE::DEAD)        statusKey = "status_death";
		else if (player->GetState() == PLAYER_STATE::ALMOST_DEAD) statusKey = "status_almost_dead";
		else if (player->GetIsPossessed())                       statusKey = "status_possessed";

		if (statusKey) {
			ImTextureID statusTex = CImGuiManager::GetInstance().GetTexture(statusKey);
			if (statusTex) {
				const float iconSize = 18.f * sy;
				const float iconX = barX + 5.f * sx;           // HP바 왼쪽에서 오른쪽으로 약간
				const float iconY = barY - iconSize - 6.f * sy; // HP바 위 (배지 여백 포함)

				// 배경 배지: 검정 아이콘이 또렷하게 보이도록 / 사망만 빨강, 그 외는 밝은 파랑
				const float  pad        = 4.f * sy;
				const float  badgeRound = 6.f * sy;
				const ImVec2 badgeMin = ImVec2(iconX - pad, iconY - pad);
				const ImVec2 badgeMax = ImVec2(iconX + iconSize + pad, iconY + iconSize + pad);
				const ImVec4 badgeColor = (player->GetState() == PLAYER_STATE::DEAD)
					? ImVec4(0.92f, 0.30f, 0.30f, 0.95f)   // 사망: 빨강
					: ImVec4(0.82f, 0.88f, 1.0f, 0.95f);   // 그 외: 밝은 파랑
				dl->AddRectFilled(badgeMin, badgeMax,
					ImGui::GetColorU32(badgeColor), badgeRound);

				// 아이콘 (배지 위)
				dl->AddImage(statusTex,
					ImVec2(iconX, iconY),
					ImVec2(iconX + iconSize, iconY + iconSize));
			}
		}

		drawn++;
	}
}

void CGameScene::DrawGiveUpButton()
{
	// 멀티 전용, 본인이 빈사(ALMOST_DEAD) 상태일 때만 표시
	if (g_is_single) return;
	if (!my_player) return;
	if (my_player->GetState() != PLAYER_STATE::ALMOST_DEAD) 
		return;

	ImGuiIO& io = ImGui::GetIO();
	const float scale = G_RATIO_Y;

	// 버튼 윈도우 (임시 위치: 화면 중앙)
	const float btnW   = 160.f * G_RATIO_X;
	const float btnH   = 70.f  * G_RATIO_Y;
	const float winPad = 10.f  * scale;
	const ImVec2 winSize{ btnW + winPad * 2, btnH + winPad * 2 };
	const ImVec2 winPos{
		io.DisplaySize.x * 0.8f - winSize.x * 0.8f,
		io.DisplaySize.y * 0.5f - winSize.y * 0.5f
	};

	ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);

	ImGuiWindowFlags wFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
	                        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
	                        | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground
	                        | ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("##GiveUpRescueWnd", nullptr, wFlags)) {
		ImGui::SetWindowFontScale(1.3f * scale);

		// "구조 포기" 이미지 버튼 (프레임/배경 투명, 호버 시 약간 밝게)
		ImTextureID giveupTex = CImGuiManager::GetInstance().GetTexture("giveup");
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.15f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1, 1, 1, 0.30f));
		bool clicked = ImGui::ImageButton("##GiveUpBtn", giveupTex, ImVec2(btnW, btnH));
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
		if (clicked) {
			ImGui::OpenPopup("##GiveUpRescueConfirm");
		}

		// 확인 모달 (같은 ID 스택 안에서 BeginPopupModal 호출해야 OpenPopup과 매칭)
		const ImVec2 modalSize{ 260.f * G_RATIO_X, 140.f * G_RATIO_Y };
		ImGui::SetNextWindowSize(modalSize, ImGuiCond_Always);
		ImGui::SetNextWindowPos(
			ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
			ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		ImGuiWindowFlags mFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
		                        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::BeginPopupModal("##GiveUpRescueConfirm", nullptr, mFlags)) {
			ImGui::SetWindowFontScale(1.3f * scale);
			// 메시지 (가로 가운데 정렬) — "정말 포기하시겠습니까?"
			const char* msg = "\xEC\xA0\x95\xEB\xA7\x90 \xED\x8F\xAC\xEA\xB8\xB0\xED\x95\x98\xEC\x8B\x9C\xEA\xB2\xA0\xEC\x8A\xB5\xEB\x8B\x88\xEA\xB9\x8C?";
			float msgW = ImGui::CalcTextSize(msg).x;
			ImGui::SetCursorPosX((modalSize.x - msgW) * 0.5f);
			ImGui::SetCursorPosY(25.f * scale);
			ImGui::TextUnformatted(msg);

			// 버튼 행 (하단 가운데 정렬)
			const float cBtnW = 90.f * G_RATIO_X;
			const float cBtnH = 35.f * G_RATIO_Y;
			const float gap   = 20.f * G_RATIO_X;
			const float totalW = cBtnW * 2 + gap;
			ImGui::SetCursorPosX((modalSize.x - totalW) * 0.5f);
			ImGui::SetCursorPosY(modalSize.y - cBtnH - 15.f * scale);

			// "예"
			if (ImGui::Button("\xEC\x98\x88", ImVec2(cBtnW, cBtnH))) {
				SendGiveUpRescue();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine(0, gap);
			// "아니오"
			if (ImGui::Button("\xEC\x95\x84\xEB\x8B\x88\xEC\x98\xA4", ImVec2(cBtnW, cBtnH))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void CGameScene::SendGiveUpRescue()
{
	if (!my_player) 
		return;

	C_GiveUpRescue pkt;
	pkt.player_id = my_player->GetID();

	auto sendBuffer = MAKE_SEND_BUFFER(pkt);
	if (auto s = my_player->GetSession()) {
		s->DoSend(sendBuffer);
	}
}

void CGameScene::Exit()
{
	CScene::Exit();

	// 라운드 타이머 정리
	if (g_is_single) {
		round_active = false;
		round_timer = 0.f;
	}

	// 복귀존 정리 (싱글/멀티 모두 유효)
	return_active = false;
	return_center = {};
	return_range  = 0.f;
	return_toasts.clear();
	my_player->SetReturned(false);

	// 정산 모달 정리
	show_settlement_modal = false;
	settlement_result     = SettlementResult{};

	// 퀵슬롯 초기화
	my_player->GetQuickSlot()->Reset();

	// 빈사 3인칭 궤도 카메라 잔재 정리
	if (camera) {
		camera->SetTarget(my_player.get());
		camera->SetOrbitMode(false);
		camera->SetCameraOffset(XMFLOAT3{ 0.0f, 0.0f, 0.0f });
	}

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
	monster_spawn_info.clear();
	id_To_Index.clear();
	player_slot_ids.clear();	// 다음 라운드 재진입 시 상대 UI 중복(누적) 방지

	objects = factory->CreateGameSceneByServer(instance_data);
	instance_data.clear();	// 다음 라운드 재진입 시 누적 방지
}

void CGameScene::Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt)
{
	XMFLOAT3 pos{ pkt.x, pkt.y, pkt.z };
	SpawnWorldItem(pkt.item_id, pkt.item_world_id, pos, pkt.durability);
}

// 아이템 리스트 (가변인자)
void CGameScene::Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt)
{
	S_Spawn_Item_List::ItemList itemList = pkt.GetItemList();

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
	auto it = std::find_if(objects.begin(), objects.end(), [&](const std::shared_ptr<CObject>& obj) {
		return obj->GetID() == pkt.item_world_id;
		});

	if (it == objects.end())
		return;

	auto worldItem = static_cast<CWorldItem*>(it->get());
	if (pkt.durability > 0) {
		auto equipment = std::static_pointer_cast<CEquipment>(worldItem->GetItem());
		equipment->SetCurrentDurability((uint16)pkt.durability);
	}

	my_player->GetInventory()->AddItemWithId(worldItem->GetItem(), pkt.inventory_id);
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

			animator->PlayAction("Ganga_search", true);
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
			animator->PlayAction("Ganga_search", true);
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

		assert(mineableList[i].type != MINEABLEOBJECT_TYPE::NONE);
		treasures.push_back(TreasureInfo{ mineableList[i].world_id, pos, mineableList[i].type });
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

			// 도구·무기 모두 내구도를 가질 수 있다. CEquipment 공통으로 처리
			if (pkt.item_type == ITEM_TYPE::EQUIPMENT) {
				auto equip = std::static_pointer_cast<CEquipment>(it->second);
				equip->SetCurrentDurability(pkt.current_durability);
			}
		}
	}
}

void CGameScene::Handle_S_PlaySound(std::shared_ptr<Session> session, S_PlaySound& pkt)
{
	if (pkt.is_global) {
		CSoundManager::GetInstance().Play((SOUND_ID)pkt.sound_id);
		return;
	}

	float volume = 1.0f;
	if (pkt.volume > 0)
		volume = pkt.volume;

	if (pkt.player_id != 0 && my_player->GetID() == pkt.player_id) {
		CSoundManager::GetInstance().Play((SOUND_ID)pkt.sound_id, 1, volume);
		return;
	}
	else {
		XMFLOAT3 soundPos = { pkt.x, pkt.y, pkt.z };
		XMFLOAT3 myPos = my_player->GetPosition();
		XMFLOAT3 diff = Vector3::Subtract(soundPos, myPos);
		diff.y = 0.0f;

		if(pkt.range > 0.f){
			if (Vector3::Length(diff) <= pkt.range)
				CSoundManager::GetInstance().Play((SOUND_ID)pkt.sound_id, 1, volume);
		}
		else {
			if (Vector3::Length(diff) <= 4.0f)
				CSoundManager::GetInstance().Play((SOUND_ID)pkt.sound_id, 1, volume);
		}
	}
}

void CGameScene::Handle_S_CHoldFail(std::shared_ptr<Session> session, S_CHoldFail& pkt)
{
	if (my_player->GetID() != pkt.player_id)
		return;

	// C-홀드 액션(빙의 해제/빈사 소생) 실패 → 타이머·대상 모두 리셋
	my_player->ResetCHoldTimer();
	my_player->SetCHoldTarget(CHOLD_TARGET::NONE);
	CSoundManager::GetInstance().Stop(SOUND_ID::clock_alarm);
}

void CGameScene::Handle_S_ReturnZoneActive(std::shared_ptr<Session> session, const S_ReturnZoneActive& pkt)
{
	return_active = true;
	return_center = XMFLOAT3{ pkt.x, pkt.y, pkt.z };
	return_range = pkt.range;
}

void CGameScene::Handle_S_PlayerReturned(std::shared_ptr<Session> session, const S_PlayerReturned& pkt)
{
	const bool isSelf = (my_player && my_player->GetID() == pkt.player_id);

	ReturnToast toast;
	toast.is_self = isSelf;
	toast.timer = RETURN_TOAST_DURATION;

	// ImGui는 UTF-8을 요구. 소스 인코딩 의존을 피하기 위해 한글을 UTF-8 hex 바이트로 직접 작성.
	if (isSelf) {
		my_player->SetReturned(true);

		// "복귀 완료"
		toast.text = "\xEB\xB3\xB5\xEA\xB7\x80 \xEC\x99\x84\xEB\xA3\x8C";
	}
	else {
		// "%llu 플레이어 복귀"
		char buf[64];
		snprintf(buf, sizeof(buf),
			"%llu \xED\x94\x8C\xEB\xA0\x88\xEC\x9D\xB4\xEC\x96\xB4 \xEB\xB3\xB5\xEA\xB7\x80",
			(unsigned long long)pkt.player_id);
		toast.text = buf;
	}

	return_toasts.push_back(std::move(toast));
	CSoundManager::GetInstance().Play(SOUND_ID::Return);
}

void CGameScene::Handle_S_GameSettlement(std::shared_ptr<Session> session, S_GameSettlement& pkt)
{
	settlement_result = SettlementResult{};
	settlement_result.base_coin = pkt.base_coin;
	settlement_result.final_coin = pkt.final_coin;
	settlement_result.is_returned = pkt.is_returned;
	settlement_result.all_returned_bonus = pkt.all_returned_bonus;

	auto list = pkt.GetTreasureList();
	for (uint16 i = 0; i < list.Count(); ++i) {
		SettlementEntry e;
		e.item_id = list[i].item_id;
		e.price = list[i].price;
		e.count = list[i].count;

		// 이름/아이콘 조회
		auto item = ItemFactory::Create(e.item_id);
		if (item) {
			e.name = item->GetName();
			e.icon_path = item->GetIconPath();
		}
		settlement_result.entries.push_back(std::move(e));
	}

	// 소지금 반영 + 인벤토리에서 보물 제거
	if (my_player) {
		my_player->SetGold(my_player->GetGold() + pkt.final_coin);

		auto inv = my_player->GetInventory();
		if (inv) {
			std::vector<uint32> to_remove;
			for (auto& [invId, item] : inv->GetItems()) {
				if (item->GetItemType() == ITEM_TYPE::TREASURE)
					to_remove.push_back(invId);
			}
			for (auto id : to_remove)
				inv->RemoveItem(id);
		}
	}

	show_settlement_modal = true;
	CKeyManager::GetInstance().SetMouseMode(false);
	CSoundManager::GetInstance().Play(SOUND_ID::Settlement);
}