#include "pch.h"
//=================
// 서버쪽 Inventory 
//=================
#include "Inventory.h"
#include "Player.h"

CInventory::CInventory(std::shared_ptr<CPlayer> _owner)
	: owner(_owner)
{
}

CInventory::~CInventory()
{
}

void CInventory::AddItem(std::shared_ptr<CItem> item)
{
	// 보물만 무게에 영향을 준다.
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {

		// 이미 꽉 찼다면 return!
		if (current_weight >= max_weight)
			return;

		// 이 보물을 추가했을 때 최대 무게를 초과하면 거부
		if (current_weight + item->GetWeight() > max_weight)
			return;

		current_weight += item->GetWeight();
	}

	items.push_back(std::move(item));
}

void CInventory::RemoveItem(int itemID)
{
	auto it = std::find_if(items.begin(), items.end(),
		[itemID](const std::shared_ptr<CItem>& item) {
			return item->GetItemId() == itemID;
		});

	if (it != items.end()) {
		current_weight -= (*it)->GetWeight();
		items.erase(it);
	}
}
