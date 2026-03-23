#include "stdafx.h"
#include "Inventory.h"
#include "ImGuiManager.h"
#include "MyPlayer.h"

CInventory::CInventory(std::shared_ptr<CMyPlayer> _owner)
	: owner(_owner)
{
}

CInventory::~CInventory()
{
}

void CInventory::AddItem(std::shared_ptr<CItem> item)
{
	if (current_weight >= max_weight)
		return;

	if (current_weight + item->GetWeight() > max_weight)
		return;

	current_weight += item->GetWeight();
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

void CInventory::Draw()
{
	if (!is_open)
		return;

	// TODO: 인벤토리 창 ImGui 코드 작성
}

void CInventory::DrawItemTable(ITEM_TYPE type)
{
	// TODO: type에 맞는 아이템만 필터링해서 테이블 렌더링
}

void CInventory::DrawBottomBar()
{
	// TODO: 하단 용량 + 소지금 렌더링
	auto player = owner.lock();
	if (!player)
		return;

	// 용량: current_weight / max_weight un
	// 소지금: player->GetGold() 원
}