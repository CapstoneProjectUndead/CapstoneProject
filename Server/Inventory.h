#pragma once
//=================
// Server Inventory 
//=================
#include "Item.h"
#include "ItemManager.h"

class CPlayer;

class CInventory
{
public:
	CInventory(std::shared_ptr<CPlayer> owner);
	CInventory(const CInventory&) = delete;
	~CInventory();

	bool AddItem(std::shared_ptr<CItem> item);
	bool RemoveItem(uint32 inventoryId);

	uint32 GetCurrentWeight() const { return current_weight; }
	void  SetCurrentWeight(uint32 weight) { current_weight = weight; }

	uint32 GetMaxWeight() const { return max_weight; }
	void  UpgradeMaxWeight(float amount) { max_weight += amount; }

	const unordered_map<uint32, std::shared_ptr<CItem>>& GetItems() const { return items; }

private:
	weak_ptr<CPlayer>							   owner;
	unordered_map<uint32, std::shared_ptr<CItem>>  items;
	uint32										   inventory_id_counter;
	uint32										   current_weight;
	uint32										   max_weight     = 200;
};
