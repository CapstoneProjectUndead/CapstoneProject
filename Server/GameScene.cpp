#include "pch.h"
// Server쪽 GameScene
#include "GameScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
#include "HumanMonster.h"
#include "Ghost.h"
#include "DogMonster.h"
#include "ServerObjectFactory.h"
#include "MapUtils.h"
#include "Item.h"
#include "ItemFactory.h"
#include "Inventory.h"


CGameScene::CGameScene(uint32 roomId)
	: CScene(SCENE_TYPE::GAME)
	, mineable_id_counter(10000)
	, round_timer(0.f)
	, round_started(false)
	, round_ended(false)
	, return_active(false)
	, return_center{}
{

}

CGameScene::~CGameScene()
{

}

void CGameScene::Initialize()
{
	CScene::Initialize();

	// 장비(무기, 도구)
	//item_manager->SpawnItem(5, XMFLOAT3{ 2, 0, 1 });
	//item_manager->SpawnItem(9, XMFLOAT3{ 2, 0, 1.2 });
	//item_manager->SpawnItem(14, XMFLOAT3{ 2, 0, 1.3 });
	//item_manager->SpawnItem(15, XMFLOAT3{ 2, 0, 1.4 });
	//item_manager->SpawnItem(17, XMFLOAT3{ 2, 0, 1.5 });
	//item_manager->SpawnItem(19, XMFLOAT3{ 2, 0, 1.6 });

	// 소비
	//item_manager->SpawnItem(25, XMFLOAT3{ 2.1, 0, 1.4 });
	//item_manager->SpawnItem(24, XMFLOAT3{ 2.1, 0, 1.5 });
	//item_manager->SpawnItem(45, XMFLOAT3{ 2.1, 0, 1.6 });

	// 기타
	//item_manager->SpawnItem(47, XMFLOAT3{ 2.2, 0, 1.7 });
	//item_manager->SpawnItem(48, XMFLOAT3{ 2.2, 0, 1.8 });
	//item_manager->SpawnItem(49, XMFLOAT3{ 2.2, 0, 1.9 });

	// 테스트 (임시코드)
	//shared_ptr<CHumanMonster> humanMonster = static_pointer_cast<CHumanMonster>(CServerObjectFactory::CreateMonster(MON_TYPE::HUMAN_MONSTER, scene_type, GetRoom(), GetPhysicsManager()));
	//if (humanMonster) {
	//	humanMonster->SetPosition(2, 0.1f, 2.f);
	//	humanMonster->SetOriginPos({ 2, 0.1f, 2.f });
	//	humanMonster->SetScene(this);
	//	AddMonster(humanMonster);
	//}
}

void CGameScene::Update(float elapsedTime)
{
	for (auto& [id, player] : players) {
		if (player->IsIncapacitated())
			continue;
		player->ApplySeparation(players);
	}

	CScene::Update(elapsedTime);
	if (!round_ended)
		UpdateMonsters(elapsedTime);

	// 라운드 타이머 진행
	if (round_started && !round_ended) {
		round_timer -= elapsedTime;

		// 종료 60초 전 복귀존 활성화 (1회 송신)
		if (!return_active && round_timer <= RETURN_ACTIVATE_REMAIN) {
			AnnounceReturnPosition();
		}

		// 복귀존 도달 감지 (활성화 이후 매 틱)
		if (return_active) {
			DetectPlayerReturns();
		}

		// 진행 불가능 상태 조기 감지 (전원 빈사, 또는 복귀+빈사 조합 등)
		CheckEarlySettlement();

		if (!round_ended && round_timer <= 0.f) {
			round_timer = 0.f;
			round_ended = true;
			TriggerSettlement();
		}
	}
}

void CGameScene::UpdateMonsters(float elapsedTime)
{
	for (auto& info : monster_spawn_info) {
		if (!info.monster.expired())
			continue;

		if (info.respawn_timer < 0.f)
			info.respawn_timer = info.respawn_time;

		info.respawn_timer -= elapsedTime;

		if (info.respawn_timer <= 0.f) {
			auto monster = CServerObjectFactory::CreateMonster(info.type, scene_type, GetRoom(), GetPhysicsManager());
			if (!monster)
				continue;

			monster->SetPosition(info.position.x, 0.1f, info.position.z);
			monster->SetOriginPos(info.position);
			monster->SetScene(this);
			AddMonster(monster);
			info.monster = monster;
			info.respawn_time  = monster->GetRespawnTime();
			info.respawn_timer = -1.f;

			S_SpawnMonster spawnPkt;
			spawnPkt.room_id    = room_id;
			spawnPkt.scene_type = scene_type;
			spawnPkt.info = NetMonsterInfo(monster->GetID(), room_id, info.type,
				info.position.x, 0.1f, info.position.z);

			auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
			BroadCast(sendBuffer);
		}
	}
}

void CGameScene::AnnounceReturnPosition()
{
	return_active = true;
	return_center = MapGenerator::GetManholePosition();

	S_ReturnZoneActive pkt;
	pkt.x = return_center.x;
	pkt.y = return_center.y;
	pkt.z = return_center.z;
	pkt.range = RETURN_RANGE;
	pkt.scene_type = scene_type;

	auto sendBuffer = MAKE_SEND_BUFFER(pkt);
	BroadCast(sendBuffer);
}

void CGameScene::DetectPlayerReturns()
{
	const float rangeSq = RETURN_RANGE * RETURN_RANGE;

	for (auto& [id, player] : players) {
		if (!player || player->GetReturned())
			continue;

		const XMFLOAT3& pp = player->GetPosition();
		float dx = pp.x - return_center.x;
		float dz = pp.z - return_center.z;

		if (dx * dx + dz * dz <= rangeSq) {
			player->SetReturned(true);
			player->SetState(PLAYER_STATE::IDLE);   // 복귀 즉시 RUN 애니메이션 정지 (이후 입력 없어 IDLE 유지)
			player->SetVelocity(0.f, 0.f, 0.f);     // 잔여 속도 제거 → 위치 진동/화면 흔들림 차단

			S_PlayerReturned pkt;
			pkt.player_id  = id;
			pkt.scene_type = scene_type;

			auto sendBuffer = MAKE_SEND_BUFFER(pkt);
			BroadCast(sendBuffer);
		}
	}
	// 전원 복귀 + 빈사/복귀 혼합 조기 정산 트리거는 CheckEarlySettlement에서 일괄 처리
}

void CGameScene::CheckEarlySettlement()
{
	if (round_ended || players.empty())
		return;

	// 모든 플레이어가 (복귀 OR 빈사) 상태이면 더 이상 라운드 진행 불가능 → 즉시 정산
	// - 전원 복귀:        기존 동작 유지 (보너스 2배는 TriggerSettlement에서 별도 계산)
	// - 전원 빈사:        구조해줄 사람 없음
	// - 복귀 + 빈사 조합: 구조해줄 활동 가능한 플레이어 없음
	for (auto& [id, player] : players) {
		if (!player)
			continue;
		if (!player->GetReturned() && !player->IsIncapacitated())
			return;  // 아직 활동 가능한 플레이어가 있음
	}

	round_timer = 0.f;
	round_ended = true;
	TriggerSettlement();
}

void CGameScene::TriggerSettlement()
{
	// 전원 복귀 여부
	bool all_returned = !players.empty();
	for (auto& [id, player] : players) {
		if (!player->GetReturned()) {
			all_returned = false;
			break;
		}
	}

	for (auto& [id, player] : players) {
		auto inv = player->GetInventory();
		if (!inv)
			continue;

		// item_id별 묶음 (price, count)
		map<uint16, std::pair<uint32, uint16>> grouped; // [item_id] → (price, count)
		uint32 base_coin = 0;

		vector<uint32> to_remove;
		for (auto& [invId, item] : inv->GetItems()) {
			if (item->GetItemType() != ITEM_TYPE::TREASURE)
				continue;

			auto treasure = static_pointer_cast<CTreasure>(item);
			uint16 iid    = static_cast<uint16>(treasure->GetItemId());
			uint32 price  = treasure->GetPrice();

			grouped[iid].first  = price;
			grouped[iid].second += 1;
			base_coin           += price;
			to_remove.push_back(invId);
		}

		for (auto invId : to_remove)
			inv->RemoveItem(invId);

		float rate      = player->GetReturned() ? 1.0f : 0.5f;
		float bonus     = all_returned ? 2.0f : 1.0f;
		uint32 final_coin = static_cast<uint32>(base_coin * rate * bonus);
		player->AddCoin(final_coin);

		// 플레이어에게 개별 정산 패킷 전송
		uint16 entry_count = static_cast<uint16>(grouped.size());
		S_GAMESETTLEMENT_WRITE writer(base_coin, final_coin,
		                              player->GetReturned(), all_returned,
		                              scene_type);

		auto list = writer.ReserveTreasureList(entry_count);

		uint16 i = 0;
		for (auto& [iid, pc] : grouped) {
			list[i].item_id = iid;
			list[i].price   = pc.first;
			list[i].count   = pc.second;
			++i;
		}

		auto sendBuffer = writer.CloseAndReturn();
		if (auto session = player->GetSession())
			session->DoSend(sendBuffer);
	}

	// 정산 완료 후 전원 복귀 상태로 전환 → 몬스터 타겟에서 제외
	for (auto& [id, player] : players) {
		if (player && !player->GetReturned())
			player->SetReturned(true);
	}
}

void CGameScene::OnSceneActivate()
{
	CScene::OnSceneActivate();

	// 첫 번째 플레이어 입장 시에만 몬스터 스폰
	// active_player_count는 CScene::OnSceneActivate()에서 증가하므로 > 1이면 이미 스폰된 상태
	if (active_player_count > 1)
		return;

	// 라운드 타이머 시작
	round_timer = ROUND_DURATION;
	round_started = true;
	round_ended = false;
	return_active = false;
	return_center = {};

	for (auto& info : monster_spawn_info) {
		auto monster = CServerObjectFactory::CreateMonster(info.type, scene_type, GetRoom(), GetPhysicsManager());
		if (!monster)
			continue;

		monster->SetPosition(info.position.x, 0.1f, info.position.z);
		monster->SetOriginPos(info.position);
		monster->SetScene(this);
		AddMonster(monster);
		info.monster = monster;
		info.respawn_time = monster->GetRespawnTime();
	}
}

void CGameScene::OnSceneDeactivate()
{
	CScene::OnSceneDeactivate();
}

void CGameScene::EnterScene(shared_ptr<CPlayer> player)
{
	CScene::EnterScene(player);

	// 라운드 진입 시 복귀 상태 리셋 (재라운드 대비)
	player->SetReturned(false);

	//if (!player->get_items_once) {
	//	uint32 itemList[14] = { 1,5,9,14,15,16,17,19,25,24,45,47,48,49 };
	//	vector<shared_ptr<CItem>> items;
	//	for (int i = 0; i < 14; ++i) {
	//		auto item = item_manager->CreateItem(itemList[i]);
	//		items.push_back(item);
	//	}
	//
	//	S_ADDITEMLIST_WRITE pktWriter(player->GetID(), scene_type);
	//	auto list = pktWriter.ReserveItemList(items.size());
	//
	//	uint32 i = 0;
	//	for (auto& item : items) {
	//		player->GetInventory()->AddItem(item);
	//		list[i].item_id = item->GetItemId();
	//		list[i].inventory_id = item->GetInventoryID();
	//		list[i].item_type = item->GetItemType();
	//		++i;
	//	}
	//
	//	auto sendBuffer = pktWriter.CloseAndReturn();
	//	if (auto s = player->GetSession())
	//		s->DoSend(sendBuffer);
	//
	//	player->get_items_once = true;
	//}
}

void CGameScene::LeaveScene(uint64 playerId)
{
	CScene::LeaveScene(playerId);
}
 
void CGameScene::LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
	obj->SetCurrentSceneType(scene_type);
	obj->GetWorldMatrix() = node->local_matrix;

	bool isRoad = (node->name == "park_road" || node->name == "village_road" || node->name == "park_green" || node->name == "house_place");
	bool isMonsterPassable = (node->name == "streetlamp");
	bool isDoor = (node->name == "wall_1_door001" || node->name == "wall_2_door001");

	if (isRoad) {
		CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, node, EColLayer::GROUND, EColLayer::ALL_MOB);
	}
	else if (isMonsterPassable) {
		CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, node, EColLayer::OBJECT, EColLayer::PLAYER);
	}
	else if (isDoor) {
		if (!node->mesh_colliders.empty()) {
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConcaveMeshShape>(node->mesh_colliders[0].positions, node->mesh_colliders[0].indices);
			auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
			collider->SetFillter({ EColLayer::OBJECT, EColLayer::ALL_MOB });
			obj->SetComponent(collider);
			GetPhysicsManager()->SetCollider(collider);
		}
	}
	else {
		CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, node, EColLayer::OBJECT, EColLayer::ALL_MOB);
	}

	objects.emplace(node->name, std::move(obj));
}

void CGameScene::LoadGameScene()
{
	if (!prototypes.empty()) 
		return;

	CMapAssetManager::GetInstance().initialize();
	if (!prototypes.empty()) return;
	{
		std::string fileName{ "../Modeling/all_map.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/all_map_2.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/map_all_3.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/map_all_4.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/obj_treasure.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/stone_treasure.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
	}
}

void CGameScene::CreateGameScene()
{
	if (prototypes.empty())
		LoadGameScene();

	// 이전 라운드 누적 상태 초기화 (재진입 대비)
	map_instance_data.clear();
	static_objects.clear();
	mineable_objects.clear();
	mineable_id_counter = 10000;
	monsters.clear();
	monster_cnt = 0;

	vector<MapGenerator::InstanceData> instanceData = MapGenerator::Generate3DMap();

	// 몬스터 스폰 위치 추출 (서버만 사용)
	monster_spawn_info.clear();

	for (auto& inst : instanceData) {
		if (inst.type == MapGenerator::EModelType::MONSTER_HUMAN)
			monster_spawn_info.push_back({ inst.position, MON_TYPE::HUMAN_MONSTER, 0.f, -1.f, {} });
		else if (inst.type == MapGenerator::EModelType::MONSTER_GHOST)
			monster_spawn_info.push_back({ inst.position, MON_TYPE::GHOST, 0.f, -1.f, {} });
		else if (inst.type == MapGenerator::EModelType::MONSTER_DOG)
			monster_spawn_info.push_back({ inst.position, MON_TYPE::ANIMAL_MONSTER, 0.f, -1.f, {} });
	}

	for (auto& inst : instanceData) {
		std::vector<std::string> meshNames = CMapAssetManager::GetInstance().GetMeshNames(inst.type);
		for (const std::string& name : meshNames) {
			if (!prototypes.contains(name)) continue;

			EModelVariant model = CMapAssetManager::GetInstance().GetVariantFromName(name);

			// 오브젝트별 렌더링 데이터를 별도 벡터에 저장 (셀당 여러 오브젝트 지원)
			MapGenerator::InstanceData rd;
			rd.position = inst.position;
			rd.rotationY = inst.rotationY;
			rd.type = inst.type;
			rd.model = model;
			map_instance_data.push_back(rd);

			auto proto = prototypes[name];
			auto collider = proto->GetComponent<CColliderComponent>();

			// TREASURE / TREASURE_HIDDEN은 CMineableObject로, 나머지는 일반 STATIC_OBJECT로 생성
			bool isMineable = (inst.type == MapGenerator::EModelType::TREASURE || inst.type == MapGenerator::EModelType::TREASURE_HIDDEN ||
				inst.type == MapGenerator::EModelType::TREASURE_VILLAGE);
			bool isHidden = (inst.type == MapGenerator::EModelType::TREASURE_HIDDEN);

			shared_ptr<CObject> obj;
			if (isMineable) {
				auto mineableType = isHidden ? MINEABLEOBJECT_TYPE::NONE_VISIBLE : MINEABLEOBJECT_TYPE::VISIBLE;
				auto mineable = make_shared<CMineableObject>(mineableType);
				mineable->SetID(mineable_id_counter);
				mineable_objects[mineable_id_counter] = mineable;
				++mineable_id_counter;
				obj = mineable;
			}
			else {
				obj = make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
			}
			obj->SetCurrentSceneType(scene_type);

			XMMATRIX world = XMLoadFloat4x4(&proto->GetWorldMatrix()) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
			XMStoreFloat4x4(&obj->GetWorldMatrix(), world);

			// collider copy (땅속 보물은 콜라이더 스킵 - 플레이어가 위로 지나갈 수 있게)
			if (!isHidden) {
				if (auto protoCollider = proto->GetComponent<CColliderComponent>()) {
					auto copyCollider = std::make_shared<CColliderComponent>(*protoCollider);
					obj->SetComponent(copyCollider);
					copyCollider->Update(0.0f);
					if (isMineable)
						copyCollider->SetFillter({ copyCollider->GetCollisionFilter().category, EColLayer::PLAYER});
					GetPhysicsManager()->SetCollider(copyCollider);
				}
			}

			// 파괴 불가 오브젝트만 static_objects로, mineable은 별도 관리
			if (!isMineable)
				static_objects.push_back(obj);
		}
	}
}

XMFLOAT3 CGameScene::FindSpawnPoint() const
{
	constexpr float TILE_SIZE = 2.0f;
	float minDist = FLT_MAX;
	int bestX = 1, bestY = 1;

	for (int y = 0; y < MapGenerator::HEIGHT; ++y) {
		for (int x = 0; x < MapGenerator::WIDTH; ++x) {
			if (!MapGenerator::IsWalkableFloor(x, y)) continue;
			if (MapGenerator::IsBlockedObject(x, y)) continue;
			if (MapGenerator::IsBlockedStructure(x, y)) continue;

			float wx = x * TILE_SIZE;
			float wz = y * TILE_SIZE;
			float dist = wx * wx + wz * wz;
			if (dist < minDist) {
				minDist = dist;
				bestX = x;
				bestY = y;
			}
		}
	}

	return XMFLOAT3{ bestX * TILE_SIZE, 2.0f, bestY * TILE_SIZE };
}

CMineableObject* CGameScene::FindNearestMineable(const XMFLOAT3& pos, float range, MINEABLEOBJECT_TYPE type)
{
	CMineableObject* nearest = nullptr;
	float min_dist_sq = range * range;

	for (auto& [id, obj] : mineable_objects) {

		if (!obj || obj->IsDestroyed())
			continue;
		if (obj->GetMineableType() != type)
			continue;

		XMFLOAT3 op = obj->GetPosition();
		float dx = op.x - pos.x;
		float dz = op.z - pos.z;
		float dist_sq = dx * dx + dz * dz;

		if (dist_sq < min_dist_sq) {
			min_dist_sq = dist_sq;
			nearest = obj.get();
		}
	}

	return nearest;
}

void CGameScene::DestroyMineable(uint32 world_id)
{
	auto it = mineable_objects.find(world_id);
	if (it == mineable_objects.end())
		return;

	XMFLOAT3 pos = it->second->GetPosition();

	// 물리 콜라이더 제거
	if (auto col = it->second->GetComponent<CColliderComponent>())
		GetPhysicsManager()->EraseCollider(col);

	// S_MineableDestroy 브로드캐스트
	S_DestroyMineable destroyMineablePkt;
	destroyMineablePkt.obj_id = world_id;
	destroyMineablePkt.scene_type = scene_type;

	auto sendBuffer = MAKE_SEND_BUFFER(destroyMineablePkt);
	BroadCast(sendBuffer);

	mineable_objects.erase(it);

	// 드롭 아이템 스폰 (보물) (임시)
	// 여기는 보물 확률 계산으로 다시 수정되어야 하는 부분
	uint16 picked = g_TreasureTable[rand() % g_TreasureTableCount];
	auto dropped = item_manager->SpawnItem(picked, pos);

	S_SpawnItem spawnPkt;
	spawnPkt.item_id = picked;
	spawnPkt.item_world_id = dropped->world_id;
	spawnPkt.item_type = ITEM_TYPE::TREASURE;
	spawnPkt.scene_type = scene_type;
	spawnPkt.x = pos.x;
	spawnPkt.y = 0.05f;
	spawnPkt.z = pos.z;

	sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
	BroadCast(sendBuffer);
}

void CGameScene::Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt)
{
	// 지금 서버 구조가 게임 로직은 싱글 스레드로 돌리고 있다.
	// 그래서 클라의 C_Pickup_Item 패킷 처리를 오는 순서대로 처리하기 때문에
	// 먼저 Pickup_Item 패킷 들어온 유저가 아이템을 소유한다.

	// 빈사/사망 상태 플레이어는 아이템 줍기 불가
	if (auto& p = players[pkt.player_id]; !p || p->IsIncapacitated())
		return;

	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		auto& player = players[pkt.player_id];
		if (!player)
			return;

		if (auto it = item_manager->FindItem(pkt.item_world_id)) {

			if (player->GetInventory()->AddItem(it)) {

				// TODO: 어떤 보물이 떳는지 확률 계산해서, 결정된 보물 ID를 사용해야한다.

				S_AddItem addItem;
				addItem.item_id       = it->GetItemId();  // 이 부분은 나중에 수정되어야 한다.
				addItem.item_world_id = pkt.item_world_id;
				addItem.inventory_id  = it->GetInventoryID();
				addItem.player_id     = pkt.player_id;
				addItem.item_type     = ITEM_TYPE::TREASURE;
				addItem.scene_type    = scene_type;

				SendBufferRef sendBuffer = MAKE_SEND_BUFFER(addItem);
				session->DoSend(sendBuffer);

				S_DeSpawnItem despawnItem;
				despawnItem.item_type     = ITEM_TYPE::TREASURE;
				despawnItem.item_world_id = pkt.item_world_id;
				despawnItem.scene_type    = scene_type;

				sendBuffer = MAKE_SEND_BUFFER(despawnItem);
				BroadCast(sendBuffer);

				item_manager->RemoveItem(pkt.item_world_id);
			}
		}
	}
	else {

		auto it = item_manager->FindItem(pkt.item_world_id);

		// 너가 처음 주운 유저다.
		if (it) {

			// 아이템 주운 플레이어
			auto& player = players[pkt.player_id];

			// 주운 플레이어의 인벤토리에 아이템을 등록
			if (player->GetInventory()->AddItem(it)) {

				// 아이템 도감 번호
				uint16 itemID = it->GetItemId();
				int16  durability = -1;

				if (it->GetItemType() == ITEM_TYPE::EQUIPMENT) {
					auto equipment = static_pointer_cast<CEquipment>(it);
					durability = equipment->GetCurrentDurability();
				}

				// S_AddItem 패킷 보낸다.
				S_AddItem addItem;
				addItem.item_id = itemID;
				addItem.item_world_id = pkt.item_world_id;
				addItem.inventory_id = it->GetInventoryID();
				addItem.player_id = pkt.player_id;
				addItem.item_type = pkt.item_type;
				addItem.durability = durability;
				addItem.scene_type = scene_type;

				SendBufferRef sendBuffer = MAKE_SEND_BUFFER(addItem);
				session->DoSend(sendBuffer);

				// S_DeSpawnItem
				S_DeSpawnItem despawnItem;
				despawnItem.item_type = pkt.item_type;
				despawnItem.item_world_id = pkt.item_world_id;
				despawnItem.scene_type = scene_type;

				sendBuffer = MAKE_SEND_BUFFER(despawnItem);
				BroadCast(sendBuffer);

				// 필드에서 플레이어가 주운 아이템 삭제
				if (item_manager->RemoveItem(pkt.item_world_id) == false) {
					// 디버깅용
					// 월드 아이디에 문제 없다면 if문 안으로 들어오면 안된다. 
					assert(nullptr);
				}
			}
		}
	}
}

void CGameScene::Handle_C_Drop_Item(shared_ptr<Session> session, const C_DropItem& pkt)
{
	auto& player = players[pkt.player_id];
	if (!player || player->IsIncapacitated())
		return;

	auto inv = player->GetInventory();

	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		if (inv) {

			// 플레이어 인벤토리에 아이템 검색
			auto item = inv->GetItems().find(pkt.inventory_id);

			// 아이템이 존재한다면
			if (item != inv->GetItems().end()) {

				uint16 itemId       = item->second->GetItemId();
				uint32 inventoryId  = item->second->GetInventoryID();

				// 인벤토리에서 아이템 제거
				if (inv->RemoveItem(pkt.inventory_id)) {

					// 인벤토리 제거 패킷 → 드롭한 플레이어에게
					S_RemoveItem removeItemPkt;
					removeItemPkt.item_id      = itemId;
					removeItemPkt.inventory_id = inventoryId;
					removeItemPkt.player_id    = player->GetID();
					removeItemPkt.scene_type   = scene_type;

					if (player->GetSession()) {
						auto sendBuffer = MAKE_SEND_BUFFER(removeItemPkt);
						player->GetSession()->DoSend(sendBuffer);
					}

					// 월드에 스폰 → 전체 브로드캐스트
					XMFLOAT3 pos      = player->GetPosition();
					float    yawRad   = XMConvertToRadians(player->GetYaw());
					pos.x            += sinf(yawRad) * 0.4f;
					pos.z            += cosf(yawRad) * 0.4f;
					pos.y             = max(pos.y, 0.0f);
					auto spawnedItem  = item_manager->SpawnItem(itemId, pos);

					S_SpawnItem spawnItemPkt;
					spawnItemPkt.item_id       = itemId;
					spawnItemPkt.item_world_id = spawnedItem->world_id;
					spawnItemPkt.item_type     = ITEM_TYPE::TREASURE;
					spawnItemPkt.scene_type    = scene_type;
					spawnItemPkt.x             = pos.x;
					spawnItemPkt.y             = pos.y;
					spawnItemPkt.z             = pos.z;

					auto sendBuffer = MAKE_SEND_BUFFER(spawnItemPkt);
					BroadCast(sendBuffer);
				}

			}
		}
	}
	else {

		if (inv) {
			auto item = inv->GetItems().find(pkt.inventory_id);

			if (item != inv->GetItems().end()) {

				uint16 itemId      = item->second->GetItemId();
				uint32 inventoryId = item->second->GetInventoryID();
				auto   dropItem    = item->second;
				int16 durability   = -1;

				if (item->second->GetItemType() == ITEM_TYPE::EQUIPMENT) {
					auto equipment = static_pointer_cast<CEquipment>(item->second);
					durability = equipment->GetCurrentDurability();
				}

				if (inv->RemoveItem(pkt.inventory_id)) {

					// 인벤토리 제거 패킷 → 드롭한 플레이어에게
					S_RemoveItem removeItemPkt;
					removeItemPkt.item_id      = itemId;
					removeItemPkt.inventory_id = inventoryId;
					removeItemPkt.player_id    = player->GetID();
					removeItemPkt.scene_type   = scene_type;

					if (player->GetSession()) {
						auto sendBuffer = MAKE_SEND_BUFFER(removeItemPkt);
						player->GetSession()->DoSend(sendBuffer);
					}

					// 월드에 스폰 → 전체 브로드캐스트
					XMFLOAT3 pos      = player->GetPosition();
					float    yawRad   = XMConvertToRadians(player->GetYaw());
					pos.x            += sinf(yawRad) * 0.4f;
					pos.z            += cosf(yawRad) * 0.4f;
					pos.y             = max(pos.y, 0.0f);
					auto spawnedItem  = item_manager->SpawnItem(itemId, pos, durability);

					S_SpawnItem spawnItemPkt;
					spawnItemPkt.item_id       = itemId;
					spawnItemPkt.item_world_id = spawnedItem->world_id;
					spawnItemPkt.item_type     = pkt.item_type;
					spawnItemPkt.scene_type    = scene_type;
					spawnItemPkt.x             = pos.x;
					spawnItemPkt.y             = pos.y;
					spawnItemPkt.z             = pos.z;
					spawnItemPkt.durability    = durability;

					auto sendBuffer = MAKE_SEND_BUFFER(spawnItemPkt);
					BroadCast(sendBuffer);

					// 드롭한 아이템이 장착 중이었으면 해제 브로드캐스트
					if (player->GetEquippedItemId() == itemId) {
						player->SetEquippedItemId(0);
						player->SetEquippedItemSubType(ITEM_SUB_TYPE::NONE);
						player->SetEquipedItem(nullptr);
						S_EquipItem unequipPkt;
						unequipPkt.player_id  = player->GetID();
						unequipPkt.item_id    = 0;
						unequipPkt.scene_type = scene_type;
						auto unequipBuffer = MAKE_SEND_BUFFER(unequipPkt);
						BroadCast(unequipBuffer);
					}
				}
			}
		}
	}
}

void CGameScene::Handle_C_Equip_Item(shared_ptr<Session> session, const C_EquipItem& pkt)
{
	auto& player = players[pkt.player_id];

	if (!player || player->GetIsPossessed() || player->IsIncapacitated())
		return;

	bool wasDowsing = player->GetDowsing();

	// IDLE -> 다우징 요청
	if (!wasDowsing && pkt.is_dowsing_rod && pkt.item_id < 0) {
		player->SetDowsing(true);

		S_EquipItem equipPkt;
		equipPkt.is_dowsing_rod = true;
		equipPkt.player_id = pkt.player_id;
		equipPkt.item_id = -1;
		equipPkt.scene_type = scene_type;

		auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
		BroadCast(sendBuffer);
		return;
	}
	// 다우징 -> IDLE 요청
	else if (wasDowsing && !pkt.is_dowsing_rod && pkt.item_id < 0) {
		player->SetDowsing(false);
		
		S_EquipItem equipPkt;
		equipPkt.is_dowsing_rod = false;
		equipPkt.player_id = pkt.player_id;
		equipPkt.item_id = -1;
		equipPkt.scene_type = scene_type;

		auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
		BroadCast(sendBuffer);
		return;
	}

	if (pkt.item_id != 0) {
		auto inventory = player->GetInventory();
		if (!inventory)
			return;

		auto& items = inventory->GetItems();
		auto iter = items.find(pkt.inventory_id);
		if (iter == items.end())
			return;

		player->SetEquippedItemId(pkt.item_id);
		player->SetEquippedItemSubType(iter->second->GetSubType());
		player->SetEquipedItem(iter->second);
	}
	else {
		// item_id == 0 : 장착 해제
		player->SetEquippedItemId(0);
		player->SetEquippedItemSubType(ITEM_SUB_TYPE::NONE);
		player->SetEquipedItem(nullptr);
	}

	S_EquipItem equipPkt;
	equipPkt.player_id  = pkt.player_id;
	equipPkt.item_id    = pkt.item_id;
	equipPkt.scene_type = scene_type;

	auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
	BroadCast(sendBuffer);
}

void CGameScene::Handle_C_Use_Item(shared_ptr<Session> session, const C_UseItem& pkt)
{
	auto& player = players[pkt.player_id];
	if (!player)
		return;

	const auto& inventory = player->GetInventory();
	if (!inventory)
		return;

	S_UseItem useItemPkt;

	auto it = inventory->GetItems().find(pkt.inventory_id);
	if (it == inventory->GetItems().end()) {
		useItemPkt.success = false;
		useItemPkt.player_id = player->GetID();
		useItemPkt.scene_type = scene_type;

		auto sendBuffer = MAKE_SEND_BUFFER(useItemPkt);
		session->DoSend(sendBuffer);
		return;
	}

	auto& item = it->second;

	useItemPkt.player_id  = player->GetID();
	useItemPkt.scene_type = scene_type;
	useItemPkt.success    = item->Use(player.get());

	auto sendBuffer = MAKE_SEND_BUFFER(useItemPkt);
	session->DoSend(sendBuffer);

	if (useItemPkt.success) {
		inventory->RemoveItem(pkt.inventory_id);

		S_RemoveItem removeItemPkt;
		removeItemPkt.player_id    = player->GetID();
		removeItemPkt.inventory_id = pkt.inventory_id;
		removeItemPkt.item_id      = pkt.item_id;
		removeItemPkt.scene_type   = scene_type;

		auto removeBuffer = MAKE_SEND_BUFFER(removeItemPkt);
		session->DoSend(removeBuffer);

		// 사용한 아이템이 장착 중이었으면 해제 브로드캐스트
		if (player->GetEquippedItemId() == pkt.item_id) {
			player->SetEquippedItemId(0);
			S_EquipItem unequipPkt;
			unequipPkt.player_id  = player->GetID();
			unequipPkt.item_id    = 0;
			unequipPkt.scene_type = scene_type;
			auto unequipBuffer = MAKE_SEND_BUFFER(unequipPkt);
			BroadCast(unequipBuffer);
		}
	}
}

void CGameScene::Handle_C_GiveUpRescue(shared_ptr<Session> session, const C_GiveUpRescue& pkt)
{
	auto player = CAST_CS(session)->GetUser()->GetPlayer();
	if (!player) 
		return;

	if (player->GetState() != PLAYER_STATE::ALMOST_DEAD) 
		return;

	player->SetState(PLAYER_STATE::DEAD);

	// 시체에 다른 플레이어가 막히지 않게 plr의 모든 collider mask=0
	if (auto pm = GetPhysicsManager()) {
		pm->SetMaskByOwner(player.get(), 0);
	}
}
