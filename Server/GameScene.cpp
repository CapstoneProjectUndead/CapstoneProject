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
#include "ServerObjectFactory.h"
#include "MapUtils.h"
#include "Item.h"
#include "ItemFactory.h"
#include "Inventory.h"


CGameScene::CGameScene(uint32 roomId)
	: CScene(SCENE_TYPE::GAME)
{

}

CGameScene::~CGameScene()
{

}

void CGameScene::Initialize()
{
	CScene::Initialize();

	item_manager->SpawnItem(25, XMFLOAT3{ 1, 0, 1 });
	item_manager->SpawnItem(24, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(45, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(5, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(1, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(17, XMFLOAT3{ 2, 0, 1 });

	item_manager->SpawnItem(47, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(47, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(48, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(57, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(77, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(91, XMFLOAT3{ 2, 0, 1 });
	item_manager->SpawnItem(100, XMFLOAT3{ 2, 0, 1 });
}

void CGameScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CGameScene::Enter()
{
	CScene::Enter();
}

void CGameScene::Exit()
{
	CScene::Exit();
}
 
void CGameScene::LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node)
{
	if (node->mesh.positions.empty()) return;

	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
	obj->GetWorldMatrix() = node->localMatrix;

	auto SetColliderComp = [&node, obj](std::unique_ptr<CColliderShape>& shape) {
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;
		collider->SetFillter(filter);
		obj->SetComponent(collider);
		};
	// ColliderComponent 
	bool isRoad = node->name == "park_road" || node->name == "village_road" || node->name == "park_green" || node->name == "house_place";
	if (!node->collider.positions.empty()) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(node->collider.positions);
		SetColliderComp(shape);
	}
	else if (isRoad) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(node->mesh.bounds.Extents, node->mesh.bounds.Center);
		auto boxCollider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		CollisionFilter filter;
		filter.category = EColLayer::GROUND;
		filter.mask = EColLayer::PLAYER;
		boxCollider->SetFillter(filter);
		obj->SetComponent(boxCollider);
	}

	objects.emplace(node->name, std::move(obj));
}

void CGameScene::LoadGameScene()
{
	if (!prototypes.empty()) 
		return;

	CMapAssetManager::GetInstance().initialize();

	std::string fileName{ "../Modeling/all_map.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	LoadFrameNode(prototypes, frameRoot);
	for (const auto& children : frameRoot->childrens) {
		LoadFrameNode(prototypes, children);
	}
}

void CGameScene::CreateGameScene()
{
	if (prototypes.empty()) 
		LoadGameScene();

	vector<MapGenerator::InstanceData> instanceData = MapGenerator::Generate3DMap();

	// 맵 데이터를 순회하며 보물 좌표 + ID 부여 + spawn
	item_manager->SpawnWorldTreasures(instanceData);

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

			auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
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
			static_objects.push_back(obj);
		}
	}
}

void CGameScene::Handle_C_Pickup_Item(shared_ptr<Session> session, const C_PickupItem& pkt)
{
	// 지금 서버 구조가 게임 로직은 싱글 스레드로 돌리고 있다.
	// 그래서 클라의 C_Pickup_Item 패킷 처리를 오는 순서대로 처리하기 때문에
	// 먼저 들어온 유저가 아이템을 소유한다.

	if (pkt.item_type == ITEM_TYPE::TREASURE) {

		auto& player = players[pkt.player_id];

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

				item_manager->treasure_map.erase(pkt.item_world_id); // 초기 보물이면 제거, 재드롭이면 no-op
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
					XMFLOAT3 pos         = player->GetPosition();
					pos.y                = max(pos.y, 0.0f);
					auto     spawnedItem = item_manager->SpawnItem(itemId, pos);

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
					XMFLOAT3 pos       = player->GetPosition();
					pos.y              = max(pos.y, 0.0f);
					auto     spawnedItem = item_manager->SpawnItem(itemId, pos);

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
				}
			}
		}
	}
}
