#include "pch.h"
#include "ItemManager.h"
#include "ItemFactory.h"
#include "Item.h"

CItemManager::CItemManager(SCENE_TYPE sceneType)
	: scene_type(sceneType)
	, world_id_counter(20000)
{

}

CItemManager::~CItemManager()
{

}

shared_ptr<CItem> CItemManager::CreateItem(uint16 itemId)
{
	auto item = ItemFactory::Create(itemId);

	item->SetWorldID(world_id_counter);
	items[world_id_counter] = item;
	++world_id_counter;

	return item;
}

shared_ptr<CItem> CItemManager::SpawnItem(uint16 itemId, const XMFLOAT3& pos)
{
	auto item = ItemFactory::Create(itemId);

	item->SetWorldID(world_id_counter);
	items[world_id_counter] = item;
	item->SetPosition(pos);
	++world_id_counter;

	return item;
}

shared_ptr<CItem> CItemManager::FindItem(uint64 worldId)
{
	auto it = items.find(worldId);

	if (it == items.end())
		return nullptr;

	return it->second;
}

bool CItemManager::RemoveItem(uint64 worldId)
{
	if (items.find(worldId) == items.end())
		return false;

	items.erase(worldId);
	return true;
}

void CItemManager::SpawnWorldTreasures(const vector<MapGenerator::InstanceData>& instanceData)
{
	for (const auto& inst : instanceData) {
		if (inst.type == MapGenerator::EModelType::TREASURE) {
			TreasureInfo treasureInfo{ world_id_counter++, inst.position };
			treasure_map[treasureInfo.world_id] = treasureInfo;
		}
	}
}
