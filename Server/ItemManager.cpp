#include "pch.h"
// Server에만 있는 클래스 (ItemManager)
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
	return item;
}

shared_ptr<WorldItem> CItemManager::SpawnItem(uint16 itemId, const XMFLOAT3& pos, int16 dur)
{
	auto item = CreateItem(itemId);

	shared_ptr<WorldItem> worldItem = make_shared<WorldItem>(item, world_id_counter, pos);

	if (dur > 0) {
		auto equipment = static_pointer_cast<CEquipment>(worldItem->item);
		equipment->SetCurrentDurability((uint16)dur);
	}

	items[world_id_counter] = worldItem;
	++world_id_counter;

	return worldItem;
}

shared_ptr<CItem> CItemManager::FindItem(uint64 worldId)
{
	auto it = items.find(worldId);

	if (it == items.end())
		return nullptr;

	return it->second->item;
}

bool CItemManager::RemoveItem(uint64 worldId)
{
	if (items.find(worldId) == items.end())
		return false;

	items.erase(worldId);
	return true;
}
