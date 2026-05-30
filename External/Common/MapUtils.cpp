#ifdef CLIENT
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================
#include "stdafx.h"
#else
#include "pch.h"
#endif

#include "MapUtils.h"
using namespace MapGenerator;

uint16 g_TreasureTable[]
{
    68,69,70,71,72,73,74,75,76,77,78,80,81,
};

uint16 g_DropTable[]
{
    16,
    20,21,22,23,24,25,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,
    40,41,42,43,44,45
};

void CMapAssetManager::initialize()
{
    // EModelVariant -> 파일명 매핑 (id_to_file)
    // 고정 에셋
    id_to_file[EModelVariant::PARK_ROAD] = "park_road";
    id_to_file[EModelVariant::PARK_GREEN] = "park_green";
    id_to_file[EModelVariant::VILLAGE_ROAD] = "village_road";
    id_to_file[EModelVariant::HOUSE_PLACE] = "house_place";
    id_to_file[EModelVariant::WALL_1002] = "wall_1002";
    id_to_file[EModelVariant::WALL_2001] = "wall_2001";
    id_to_file[EModelVariant::WALL_1003] = "wall_1003";
    id_to_file[EModelVariant::WALL_1_DOOR001] = "wall_1_door001";
    id_to_file[EModelVariant::WALL_2_DOOR001] = "wall_2_door001";
    id_to_file[EModelVariant::VENDING_MACHINE_001] = "vending_machine001";
    id_to_file[EModelVariant::SEESAW_001] = "seesaw001";
    id_to_file[EModelVariant::STONE_WALL] = "stone_wall";
    id_to_file[EModelVariant::PARK_WALL] = "park_wall";
    id_to_file[EModelVariant::TENT_CLOTH] = "tent_cloth";
    id_to_file[EModelVariant::TENT_CORNER_CLOTH] = "tent_corner_cloth";
    id_to_file[EModelVariant::TENT_CORNER] = "tent_corner";
    id_to_file[EModelVariant::CRATE_1] = "crate_1";
    id_to_file[EModelVariant::CRATE_2] = "crate_2";
    id_to_file[EModelVariant::DRUM] = "drum";
    id_to_file[EModelVariant::STONE_TREASURE] = "stone_treasure";

    id_to_file[EModelVariant::PARK_SAND] = "park_sand";

    // Grass (ID 반복문 처리)
    for (int i = 0; i <= 2; ++i) {
        EModelVariant var = static_cast<EModelVariant>(static_cast<int>(EModelVariant::GRASS_1) + (i));
        std::string name = "park_grass_" + std::to_string(i + 1);
        id_to_file[var] = name;
    }

    // Stone (ID 반복문 처리)
    for (int i = 11; i <= 24; ++i) {
        EModelVariant var = static_cast<EModelVariant>(static_cast<int>(EModelVariant::STONE_011) + (i - 11));
        std::string name = "stone0" + std::to_string(i);
        id_to_file[var] = name;
    }

    // Props
    id_to_file[EModelVariant::PARK_BENCH_002] = "park_bench002";
    id_to_file[EModelVariant::PARK_BENCH_003] = "park_bench003";
    id_to_file[EModelVariant::PARK_BUSH] = "park_bush";
    id_to_file[EModelVariant::PARK_SHRUB] = "park_shrub";
    id_to_file[EModelVariant::TREE_1] = "tree_1";
    id_to_file[EModelVariant::TREE_2] = "tree_2";
    id_to_file[EModelVariant::TREE_3] = "tree_3";
    id_to_file[EModelVariant::TRASHCAN_001] = "trashcan001";
    id_to_file[EModelVariant::TRASHCAN_002] = "trashcan002";
    id_to_file[EModelVariant::MANHOLE] = "manhole";

    id_to_file[EModelVariant::BOOKSHELF] = "bookshelf";
    id_to_file[EModelVariant::FENCE_WOOD] = "fence_wood";
    id_to_file[EModelVariant::REFRIGERATOR] = "refrigerator";
    id_to_file[EModelVariant::REFRIGERATOR_DOOR_LOWER] = "refrigerator_door_lower";
    id_to_file[EModelVariant::REFRIGERATOR_DOOR_UPPER] = "refrigerator_door_upper";
    id_to_file[EModelVariant::SOFA] = "sofa";
    id_to_file[EModelVariant::STREETLAMP] = "streetlamp";
    id_to_file[EModelVariant::TABLE_LOW] = "table_low";
    id_to_file[EModelVariant::CHAIR_001] = "chair001";
    id_to_file[EModelVariant::TABLE_001] = "table001";

    // 카테고리별 랜덤 풀 설정 (추가 장식물용 - 파일명 기반)
    random_pools["grass"] = { "park_grass_1", "park_grass_2", "park_grass_3"};
    random_pools["stone"] = { "stone011", "stone012", "stone013", "stone014", "stone015", "stone016", "stone017",
        "stone018", "stone019", "stone020", "stone021", "stone022", "stone023", "stone024" };
    random_pools["tree"] = { "tree_1", "tree_2", "tree_3" };
    random_pools["bench"] = { "park_bench002", "park_bench003" };
    random_pools["bush"] = { "park_bush", "park_shrub" };
    random_pools["trashcan"] = { "trashcan001", "trashcan002" };
    random_pools["indoor_furniture"] = {
        "bookshelf", "sofa", "table_low", "chair.001", "table.001", "refrigerator"
    };
    random_pools["village_deco"] = { "streetlamp" };
    random_pools["treasure"] = { "crate_1"};
    random_pools["treasure_village"] = { "crate_1", "crate_2", "drum" };
    random_pools["treasure_field"] = { "crate_1", "crate_2", "drum", "stone_treasure" };

    // 모델 타입별 메쉬 매핑 (EModelType -> {EModelVariant 후보들(실제 모델 enum), 추가 풀 키})
    asset_table[EModelType::ROAD] = { {EModelVariant::PARK_ROAD}, {} };
    asset_table[EModelType::PARK_GREEN] = { {EModelVariant::PARK_GREEN}, {} };

    asset_table[EModelType::VILLAGE_ROAD] = { {EModelVariant::VILLAGE_ROAD}, {} };
    //asset_table[EModelType::WALL] = { {EModelVariant::STONE_WALL}, {} };
    asset_table[EModelType::PARK_WALL] = { {EModelVariant::PARK_WALL}, {} };
    asset_table[EModelType::VILLAGE_WALL] = { {EModelVariant::STONE_WALL}, {} };
    asset_table[EModelType::HOUSE_INNTER] = { {EModelVariant::HOUSE_PLACE}, {} };
    asset_table[EModelType::HOUSE_WALL_STRAIGHT] = { {EModelVariant::WALL_1002}, {} };
    asset_table[EModelType::HOUSE_WALL_CORNER] = { {EModelVariant::WALL_2001}, {} };
    asset_table[EModelType::HOUSE_WALL_EMPTY] = { {EModelVariant::WALL_1003}, {} };
    asset_table[EModelType::DOOR] = { {EModelVariant::WALL_1_DOOR001}, {} };
    asset_table[EModelType::CORNER_DOOR] = { {EModelVariant::WALL_2_DOOR001}, {} };
    asset_table[EModelType::KIOSK] = { {EModelVariant::VENDING_MACHINE_001}, {} };
    asset_table[EModelType::TREE] = { {}, {"tree"} };
    asset_table[EModelType::BENCH] = { {}, {"bench"} };
    asset_table[EModelType::SMALL_BUSH] = { {}, {"bush"} };
    asset_table[EModelType::SEESAW] = { {EModelVariant::SEESAW_001}, {} };
    asset_table[EModelType::TREASURE_VILLAGE] = { {}, {"treasure_village"} };
    asset_table[EModelType::TREASURE] = { {}, {"treasure_field"} };
    asset_table[EModelType::TREASURE_HIDDEN] = { {}, {"treasure"} };

    // 땅속 보물: 프로토타입 룩업(콜라이더/위치)을 위해 동일 풀 사용. 메시는 ObjectFactory에서 스킵.
    asset_table[EModelType::TREASURE_HIDDEN] = { {}, {"treasure"} };

    asset_table[EModelType::GRASS] = { {}, {"grass"} };
    asset_table[EModelType::DECO_STONE] = { {}, {"stone"} };
    asset_table[EModelType::PARK_SAND_DECO] = { {EModelVariant::PARK_SAND}, {} };

    asset_table[EModelType::MANHOLE] = { {EModelVariant::MANHOLE} };  // entry나 manhole로 설정 필요

    // 천막 상점 관련
    asset_table[EModelType::STORE_WALL_EMPTY] = { {EModelVariant::TENT_CLOTH}, {} };
    asset_table[EModelType::STORE_WALL_CORNER] = { {EModelVariant::TENT_CORNER_CLOTH, EModelVariant::TENT_CORNER}, {} };

    // 서버 전용 마커 (렌더링 없음)
    asset_table[EModelType::MONSTER_HUMAN] = { {}, {} };
    asset_table[EModelType::MONSTER_GHOST] = { {}, {} };

    // [집 내부 바닥] 바닥 위에 가구가 랜덤하게 생성됨
    asset_table[EModelType::HOUSE_INNTER] = { {EModelVariant::HOUSE_PLACE}, {"indoor_furniture"} };

    // [마을 길] 마을 길바닥에 가로등이 가끔씩
    asset_table[EModelType::VILLAGE_ROAD] = { {EModelVariant::VILLAGE_ROAD}, {"village_deco"} };

    asset_table[EModelType::PARK_WALL] = { {EModelVariant::PARK_WALL}, {} };
    asset_table[EModelType::FENCE_WOOD_STR] = { {EModelVariant::FENCE_WOOD}, {} };
}

std::vector<std::string> CMapAssetManager::GetMeshNames(EModelType type, EModelVariant serverModelId)
{
    std::vector<std::string> results;

    if (serverModelId != EModelVariant::NONE) {
        if (id_to_file.contains(serverModelId)) {
            results.push_back(id_to_file[serverModelId]);
        }
    }
    else if (asset_table.contains(type)) {
        const auto& asset_entry = asset_table[type];

        // [메인 모델] 바닥/벽 등은 100% 생성
        if (!asset_entry.main_variants.empty()) {
            for (auto& main : asset_entry.main_variants) {
                results.push_back(id_to_file[main]);
            }
        }

        // [추가 장식물] 중복 및 확률 로직
        for (const auto& pool_key : asset_entry.extra_pools) {
            int probability = 15; // 기본 확률

            if (pool_key == "village_deco") {
                probability = 5;
            }
            else if (type == EModelType::TREASURE || type == EModelType::TREASURE_VILLAGE || type == EModelType::TREASURE_HIDDEN || 
                type == EModelType::PARK_GREEN || type == EModelType::BENCH || type == EModelType::TREE) {
                probability = 100;
            }
            else if (pool_key == "indoor_furniture") {
                probability = 45;
            }

            // 확률 체크
            if (rand() % 100 < probability) {
                if (random_pools.contains(pool_key)) {
                    const auto& pool = random_pools[pool_key];
                    if (!pool.empty()) {
                        results.push_back(pool[rand() % pool.size()]);

                        if (type == EModelType::PARK_GREEN && pool_key == "grass") {
                            results.push_back(pool[rand() % pool.size()]);
                        }
                    }
                }
            }
        }
    }
    return results;
}

EModelVariant CMapAssetManager::GetVariantFromName(const std::string& meshName)
{
    for (auto const& [variant, name] : id_to_file) {
        if (name == meshName) return variant;
    }
    return EModelVariant::NONE;
}
