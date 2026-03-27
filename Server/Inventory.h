#pragma once
//=================
// ¼­¹öÂÊ Inventory 
//=================
#include "Item.h"

class CPlayer;

class CInventory
{
public:
	CInventory(std::shared_ptr<CPlayer> owner);
	CInventory(const CInventory&) = delete;
	~CInventory();

	void AddItem(std::shared_ptr<CItem> item);
	void RemoveItem(int itemID);

	float GetCurrentWeight() const { return current_weight; }
	float GetMaxWeight() const { return max_weight; }
	void  UpgradeMaxWeight(float amount) { max_weight += amount; }

	const std::vector<std::shared_ptr<CItem>>& GetItems() const { return items; }

private:
	std::weak_ptr<CPlayer>               owner;

	std::vector<std::shared_ptr<CItem>>  items;
	float                                current_weight = 0.0f;
	float                                max_weight     = 200.0f;
};
