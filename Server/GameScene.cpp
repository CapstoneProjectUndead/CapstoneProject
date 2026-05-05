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
#include "ServerObjectFactory.h"
#include "MapUtils.h"
#include "Item.h"
#include "ItemFactory.h"
#include "Inventory.h"


CGameScene::CGameScene(uint32 roomId)
	: CScene(SCENE_TYPE::GAME)
	, mineable_id_counter(10000)
{

}

CGameScene::~CGameScene()
{

}

void CGameScene::Initialize()
{
	CScene::Initialize();

	// 장비(무기, 도구)
	item_manager->SpawnItem(5, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(9, XMFLOAT3{ 2, 0, 1.2 });
	item_manager->SpawnItem(14, XMFLOAT3{ 2, 0, 1.3 });
	item_manager->SpawnItem(15, XMFLOAT3{ 2, 0, 1.4 });
	item_manager->SpawnItem(17, XMFLOAT3{ 2, 0, 1.5 });
	item_manager->SpawnItem(19, XMFLOAT3{ 2, 0, 1.6 });

	// 소비
	item_manager->SpawnItem(25, XMFLOAT3{ 2.1, 0, 1.4 });
	item_manager->SpawnItem(24, XMFLOAT3{ 2.1, 0, 1.5 });
	item_manager->SpawnItem(45, XMFLOAT3{ 2.1, 0, 1.6 });

	// 기타
	item_manager->SpawnItem(47, XMFLOAT3{ 2.2, 0, 1.7 });
	item_manager->SpawnItem(48, XMFLOAT3{ 2.2, 0, 1.8 });
	item_manager->SpawnItem(49, XMFLOAT3{ 2.2, 0, 1.9 });

	// 테스트 (임시코드)
	//shared_ptr<CGhost> ghost = static_pointer_cast<CGhost>(CServerObjectFactory::CreateMonster(MON_TYPE::GHOST, scene_type, GetRoom(), GetPhysicsManager()));
	//if (ghost) {
	//	ghost->SetPosition(2, 0.1f, 2.f);
	//	ghost->SetOriginPos({ 2, 0.1f, 2.f });
	//	AddMonster(ghost);
	//}
}

void CGameScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CGameScene::OnSceneActivate()
{
	CScene::OnSceneActivate();

	// 첫 번째 플레이어 입장 시에만 몬스터 스폰
	// active_player_count는 CScene::OnSceneActivate()에서 증가하므로 > 1이면 이미 스폰된 상태
	if (active_player_count > 1)
		return;

	for (const auto& pos : humanMonster_spawn_positions) {
		shared_ptr<CHumanMonster> humanMonster = static_pointer_cast<CHumanMonster>(CServerObjectFactory::CreateMonster(MON_TYPE::HUMAN_MONSTER, scene_type, GetRoom(), GetPhysicsManager()));
		if (!humanMonster)
			continue;
	
		humanMonster->SetPosition(pos.x, 0.1f, pos.z);
		humanMonster->SetOriginPos({ pos.x, 0.1f, pos.z });
		humanMonster->SetScene(this);
		AddMonster(humanMonster);
	}
	
	for (const auto& pos : ghost_spawn_positions) {
		shared_ptr<CGhost> ghost = static_pointer_cast<CGhost>(CServerObjectFactory::CreateMonster(MON_TYPE::GHOST, scene_type, GetRoom(), GetPhysicsManager()));
		if (!ghost)
			continue;
	
		ghost->SetPosition(pos.x, 0.1f, pos.z);
		ghost->SetOriginPos({ pos.x, 0.1f, pos.z });
		ghost->SetScene(this);
		AddMonster(ghost);
	}
}

void CGameScene::OnSceneDeactivate()
{
	CScene::OnSceneDeactivate();

	monsters.clear();
	monster_cnt = 0;
}
 
void CGameScene::LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
	obj->GetWorldMatrix() = node->local_matrix;

	bool isRoad = (node->name == "park_road" || node->name == "village_road" || node->name == "park_green" || node->name == "house_place");

	if (isRoad) {
		CServerObjectFactory::AddCollider(GetPhysicsManager(), obj, node, EColLayer::GROUND, EColLayer::ALL_MOB);
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
}

void CGameScene::CreateGameScene()
{
	if (prototypes.empty()) 
		LoadGameScene();

	vector<MapGenerator::InstanceData> instanceData = MapGenerator::Generate3DMap();

	// 몬스터 스폰 위치 추출 (서버만 사용)
	humanMonster_spawn_positions.clear();
	ghost_spawn_positions.clear();

	for (auto& inst : instanceData) {
		if (inst.type == MapGenerator::EModelType::MONSTER_HUMAN)
			humanMonster_spawn_positions.push_back(inst.position);
		else if (inst.type == MapGenerator::EModelType::MONSTER_GHOST)
			ghost_spawn_positions.push_back(inst.position);
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

			// TREASURE는 CMineableObject로, 나머지는 일반 STATIC_OBJECT로 생성
			shared_ptr<CObject> obj;
			bool isMineable = (inst.type == MapGenerator::EModelType::TREASURE);
			if (isMineable) {
				auto mineable = make_shared<CMineableObject>();
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

			// collider copy
			if (CMapAssetManager::GetInstance().RequiresCollider(name)) {
				if (auto protoCollider = proto->GetComponent<CColliderComponent>()) {
					auto copyCollider = std::make_shared<CColliderComponent>(*protoCollider);
					obj->SetComponent(copyCollider);
					copyCollider->Update(0.0f);
					GetPhysicsManager()->SetCollider(copyCollider);
				}
			}

			// 파괴 불가 오브젝트만 static_objects로, mineable은 별도 관리
			if (!isMineable)
				static_objects.push_back(obj);
		}
	}
}

CMineableObject* CGameScene::FindNearestMineable(const XMFLOAT3& pos, float range)
{
	CMineableObject* nearest = nullptr;
	float min_dist_sq = range * range;

	for (auto& [id, obj] : mineable_objects) {

		if (!obj || obj->IsDestroyed())
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
	auto dropped = item_manager->SpawnItem(110, pos);

	S_SpawnItem spawnPkt;
	spawnPkt.item_id = 110;
	spawnPkt.item_world_id = dropped->world_id;
	spawnPkt.item_type = ITEM_TYPE::TREASURE;
	spawnPkt.scene_type = scene_type;
	spawnPkt.x = pos.x;
	spawnPkt.y = pos.y;
	spawnPkt.z = pos.z;

	sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
	BroadCast(sendBuffer);
}

void CGameScene::Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt)
{
	// 지금 서버 구조가 게임 로직은 싱글 스레드로 돌리고 있다.
	// 그래서 클라의 C_Pickup_Item 패킷 처리를 오는 순서대로 처리하기 때문에
	// 먼저 Pickup_Item 패킷 들어온 유저가 아이템을 소유한다.

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

				// S_AddItem 패킷 보낸다.
				S_AddItem addItem;
				addItem.item_id = itemID;
				addItem.item_world_id = pkt.item_world_id;
				addItem.inventory_id = it->GetInventoryID();
				addItem.player_id = pkt.player_id;
				addItem.item_type = pkt.item_type;
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
	if (!player)
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
					spawnItemPkt.item_type     = pkt.item_type;
					spawnItemPkt.scene_type    = scene_type;
					spawnItemPkt.x             = pos.x;
					spawnItemPkt.y             = pos.y;
					spawnItemPkt.z             = pos.z;

					auto sendBuffer = MAKE_SEND_BUFFER(spawnItemPkt);
					BroadCast(sendBuffer);

					// 드롭한 아이템이 장착 중이었으면 해제 브로드캐스트
					if (player->GetEquippedItemId() == itemId) {
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
		}
	}
}

void CGameScene::Handle_C_Equip_Item(shared_ptr<Session> session, const C_EquipItem& pkt)
{
	auto& player = players[pkt.player_id];
	if (!player)
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