#ifdef CLIENT
#include "stdafx.h"
#else
#include "pch.h"
#endif

#include "MapUtils.h"

std::string GetVariantFileName(EModelVariant variant)
{
	static const std::unordered_map<EModelVariant, std::string> variantToString = {
		{ EModelVariant::NONE, "" },

		// --- [고정 에셋들] ---
		{ EModelVariant::PARK_ROAD, "park_road" },
		{ EModelVariant::PARK_GREEN, "park_green" },
		{ EModelVariant::VILLAGE_ROAD, "village_road" },
		{ EModelVariant::HOUSE_PLACE, "house_place" },
		{ EModelVariant::WALL_1002, "wall_1002" },
		{ EModelVariant::WALL_2001, "wall_2001" },
		{ EModelVariant::WALL_1003, "wall_1003" },
		{ EModelVariant::WALL_1_DOOR001, "wall_1_door001" },
		{ EModelVariant::WALL_2_DOOR001, "wall_2_door001" },
		{ EModelVariant::VENDING_MACHINE_001, "vending_machine001" },
		{ EModelVariant::SEESAW_001, "seesaw001" },

		// Grass
		{ EModelVariant::GRASS_019, "grass019" }, { EModelVariant::GRASS_020, "grass020" },
		{ EModelVariant::GRASS_021, "grass021" }, { EModelVariant::GRASS_022, "grass022" },
		{ EModelVariant::GRASS_023, "grass023" }, { EModelVariant::GRASS_024, "grass024" },
		{ EModelVariant::GRASS_025, "grass025" }, { EModelVariant::GRASS_026, "grass026" },
		{ EModelVariant::GRASS_027, "grass027" }, { EModelVariant::GRASS_028, "grass028" },
		{ EModelVariant::GRASS_029, "grass029" }, { EModelVariant::GRASS_030, "grass030" },
		{ EModelVariant::GRASS_031, "grass031" }, { EModelVariant::GRASS_032, "grass032" },
		{ EModelVariant::GRASS_033, "grass033" }, { EModelVariant::GRASS_034, "grass034" },
		{ EModelVariant::GRASS_035, "grass035" }, { EModelVariant::GRASS_036, "grass036" },
		{ EModelVariant::GRASS_037, "grass037" },

		// Stone
		{ EModelVariant::STONE_011, "stone011" }, { EModelVariant::STONE_012, "stone012" },
		{ EModelVariant::STONE_013, "stone013" }, { EModelVariant::STONE_014, "stone014" },
		{ EModelVariant::STONE_015, "stone015" }, { EModelVariant::STONE_016, "stone016" },
		{ EModelVariant::STONE_017, "stone017" }, { EModelVariant::STONE_018, "stone018" },
		{ EModelVariant::STONE_019, "stone019" }, { EModelVariant::STONE_020, "stone020" },
		{ EModelVariant::STONE_021, "stone021" }, { EModelVariant::STONE_022, "stone022" },
		{ EModelVariant::STONE_023, "stone023" }, { EModelVariant::STONE_024, "stone024" },

		// Props
		{ EModelVariant::PARK_BENCH_002, "park_bench002" },
		{ EModelVariant::PARK_BENCH_003, "park_bench003" },
		{ EModelVariant::SMALL_BUSH_001, "small_bush001" },
		{ EModelVariant::SMALL_BUSH_002, "small_bush002" },
		{ EModelVariant::TREE_002, "tree002" },
		{ EModelVariant::PINETREE, "pinetree" },
		{ EModelVariant::TRASHCAN_001, "trashcan001" },
		{ EModelVariant::TRASHCAN_002, "trashcan002" }
	};

	auto it = variantToString.find(variant);
	return (it != variantToString.end()) ? it->second : "";
}

EModelVariant PickRandomVariant(const std::string& key)
{
	// 1. 랜덤 카테고리인지 먼저 확인 (기존 로직)
	static const std::unordered_map<std::string, std::vector<EModelVariant>> categoryTable = {
		{ "grass", {
			EModelVariant::GRASS_019, EModelVariant::GRASS_020, EModelVariant::GRASS_021,
			EModelVariant::GRASS_022, EModelVariant::GRASS_023, EModelVariant::GRASS_024,
			EModelVariant::GRASS_025, EModelVariant::GRASS_026, EModelVariant::GRASS_027,
			EModelVariant::GRASS_028, EModelVariant::GRASS_029, EModelVariant::GRASS_030,
			EModelVariant::GRASS_031, EModelVariant::GRASS_032, EModelVariant::GRASS_033,
			EModelVariant::GRASS_034, EModelVariant::GRASS_035, EModelVariant::GRASS_036,
			EModelVariant::GRASS_037, EModelVariant::NONE
		}},
		{ "stone", {
			EModelVariant::STONE_011, EModelVariant::STONE_012, EModelVariant::STONE_013,
			EModelVariant::STONE_014, EModelVariant::STONE_015, EModelVariant::STONE_016,
			EModelVariant::STONE_017, EModelVariant::STONE_018, EModelVariant::STONE_019,
			EModelVariant::STONE_020, EModelVariant::STONE_021, EModelVariant::STONE_022,
			EModelVariant::STONE_023, EModelVariant::STONE_024, EModelVariant::NONE
		}},
		{ "park_bench", { EModelVariant::PARK_BENCH_002, EModelVariant::PARK_BENCH_003 }},
		{ "small_bush", { EModelVariant::SMALL_BUSH_001, EModelVariant::SMALL_BUSH_002 }},
		{ "tree",       { EModelVariant::TREE_002,       EModelVariant::PINETREE }},
		{ "trashcan",   { EModelVariant::TRASHCAN_001,   EModelVariant::TRASHCAN_002 }},
	};

	auto it = categoryTable.find(key);
	if (it != categoryTable.end()) {
		const auto& list = it->second;
		return list[rand() % list.size()];
	}

	// 2. 랜덤이 아니라면? 고정 메쉬 테이블에서 검색!
	static const std::unordered_map<std::string, EModelVariant> fixedTable = {
		{ "park_road", EModelVariant::PARK_ROAD },
		{ "park_green", EModelVariant::PARK_GREEN },
		{ "village_road", EModelVariant::VILLAGE_ROAD },
		{ "house_place", EModelVariant::HOUSE_PLACE },
		{ "wall_1002", EModelVariant::WALL_1002 },
		{ "wall_2001", EModelVariant::WALL_2001 },
		{ "wall_1003", EModelVariant::WALL_1003 },
		{ "wall_1_door001", EModelVariant::WALL_1_DOOR001 },
		{ "wall_2_door001", EModelVariant::WALL_2_DOOR001 },
		{ "vending_machine001", EModelVariant::VENDING_MACHINE_001 },
		{ "seesaw001", EModelVariant::SEESAW_001 }
	};

	auto fixedIt = fixedTable.find(key);
	if (fixedIt != fixedTable.end()) {
		return fixedIt->second;
	}

	// 여기까지 왔는데도 없으면 진짜 에러거나 빈 공간
	return EModelVariant::NONE;
}