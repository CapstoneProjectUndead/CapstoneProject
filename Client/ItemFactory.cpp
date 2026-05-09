#include "stdafx.h"
#include "ItemFactory.h"
#include "Item.h"
#include <nlohmann/json.hpp>
#include <fstream>

namespace ItemFactory
{
	struct ItemEntry
	{
		ItemData       base;
		uint32         max_durability = 0;
		uint32         heal_amount    = 0;
		uint32         energy_amount  = 0;
		uint32         effect_amount  = 0;
		float          buff_duration  = 0.f;
		TREASURE_GRADE grade          = TREASURE_GRADE::COMMON;
	};

	static std::unordered_map<int, ItemEntry> item_db;
	static std::unordered_map<int, std::string> item_model_map;

	void LoadModelMap(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
			::OutputDebugStringA(("[ItemFactory] 파일을 열 수 없음: " + path + "\n").c_str());
			return;
		}

		nlohmann::json j;
		file >> j;

		// "item_models" 키가 있는지 먼저 확인
		if (j.contains("item_models")) {
			auto& models = j["item_models"];

			for (auto& [id, info] : models.items()) {
				int itemId = std::stoi(id);
				std::string modelName = info["model"];
				item_model_map[itemId] = modelName;
			}
		}
	}

	std::string GetModelName(int itemId)
	{
		if (item_model_map.find(itemId) != item_model_map.end()) {
			return item_model_map[itemId];
		}
		return "";
	}

	void LoadFromJson(const std::string& path)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
			::OutputDebugStringA(("[ItemFactory] 파일을 열 수 없음: " + path + "\n").c_str());
			return;
		}

		nlohmann::json j;
		file >> j;

		for (const auto& e : j) {
			ItemEntry entry;
			entry.base.item_id       = e["item_id"];
			entry.base.item_type     = static_cast<ITEM_TYPE>(e["item_type"].get<uint8_t>());
			entry.base.item_sub_type = static_cast<ITEM_SUB_TYPE>(e["item_sub_type"].get<uint16_t>());
			entry.base.item_name     = e["item_name"];
			entry.base.weight        = e.value("weight", 0.0f);
			entry.base.icon_path     = e.value("icon_path", std::string{});
			entry.base.mesh_path     = e.value("mesh_path", std::string{});
			entry.base.description   = e.value("description", std::string{});
			entry.base.price         = static_cast<uint32>(e.value("price", 0));
			entry.base.max_stack     = static_cast<uint16>(e.value("max_stack", 1));
			entry.base.is_throwable  = e.value("is_throwable", false);

			entry.max_durability = static_cast<uint32>(e.value("max_durability", 0));
			entry.heal_amount    = static_cast<uint32>(e.value("heal_amount",    0));
			entry.energy_amount  = static_cast<uint32>(e.value("energy_amount",  0));
			entry.effect_amount  = static_cast<uint32>(e.value("effect_amount",  0));
			entry.buff_duration  = e.value("duration", 0.f);
			entry.grade          = static_cast<TREASURE_GRADE>(e.value("grade",  0));

			item_db[entry.base.item_id] = std::move(entry);
		}
	}

	std::shared_ptr<CItem> Create(int itemID)
	{
		auto it = item_db.find(itemID);
		if (it == item_db.end()) {
			::OutputDebugStringA(("[ItemFactory] item ID 없음: " + std::to_string(itemID) + "\n").c_str());
			return nullptr;
		}

		const ItemEntry& e = it->second;
		std::shared_ptr<ItemData> data = std::make_shared<ItemData>(e.base);

		switch (e.base.item_type) {
		case ITEM_TYPE::EQUIPMENT:
			if (e.base.item_sub_type >= ITEM_SUB_TYPE::WEAPON)
				return std::make_shared<CWeapon>(data, e.max_durability);
			else
				return std::make_shared<CTool>(data, e.max_durability);
		case ITEM_TYPE::CONSUMABLE:
			return std::make_shared<CConsumable>(data, e.heal_amount, e.energy_amount, e.effect_amount, e.buff_duration);
		case ITEM_TYPE::TREASURE:
			return std::make_shared<CTreasure>(data, e.grade, e.base.price);
		case ITEM_TYPE::ETC:
			return std::make_shared<COther>(data);
		default:
			return nullptr;
		}
	}
}
