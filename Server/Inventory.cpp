#include "pch.h"
//=================
// 서버쪽 Inventory 
//=================
#include "Inventory.h"
#include "Player.h"

CInventory::CInventory(std::shared_ptr<CPlayer> _owner)
	: owner(_owner)
	, inventory_id_counter(0)
	, current_weight(0)
{
}

CInventory::~CInventory()
{
}

bool CInventory::AddItem(std::shared_ptr<CItem> item)
{
	// 보물만 무게에 영향을 준다.
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {

		// 이미 꽉 찼다면 return!
		if (current_weight >= max_weight)
			return false;

		// 이 보물을 추가했을 때 최대 무게를 초과하면 거부
		if (current_weight + item->GetWeight() > max_weight)
			return false;

		current_weight += item->GetWeight();
	}

	uint32 id = inventory_id_counter++;
	item->SetInventoryID(id);   // CItem에 inventory_id 필드 추가
	items[id] = item;

	return true;
}

void CInventory::RemoveItem(uint32 inventoryId)
{
	auto it = items.find(inventoryId);

	if (it != items.end()) {
		current_weight -= it->second->GetWeight();
		items.erase(it);
	}
}
