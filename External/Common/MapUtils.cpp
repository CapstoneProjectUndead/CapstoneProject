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

    // 카테고리별 랜덤 풀 설정 (추가 장식물용 - 파일명 기반)
    random_pools["grass"] = { "park_grass_1", "park_grass_2", "park_grass_3"};
    random_pools["stone"] = { "stone011", "stone012", "stone013", "stone014", "stone015", "stone016", "stone017",
        "stone018", "stone019", "stone020", "stone021", "stone022", "stone023", "stone024" };
    random_pools["tree"] = { "tree_1", "tree_2", "tree_3" };
    random_pools["bench"] = { "park_bench002", "park_bench003" };
    random_pools["bush"] = { "park_bush", "park_shrub" };
    random_pools["trashcan"] = { "trashcan001", "trashcan002" };

    // 모델 타입별 메쉬 매핑 (EModelType -> {EModelVariant 후보들(실제 모델 enum), 추가 풀 키})
    asset_table[EModelType::ROAD] = { {EModelVariant::PARK_ROAD}, {"stone"} };
    asset_table[EModelType::PARK_GREEN] = { {EModelVariant::PARK_GREEN}, {"grass"} };
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
    asset_table[EModelType::TREASURE] = { {}, {"trashcan"} };
    //asset_table[EModelType::MANHOLE] = { {EModelVariant::MANHOLE} };  // entry나 manhole로 설정 필요

    // 천막 상점 관련
    asset_table[EModelType::STORE_WALL_EMPTY] = { {EModelVariant::TENT_CLOTH}, {} };
    asset_table[EModelType::STORE_WALL_CORNER] = { {EModelVariant::TENT_CORNER_CLOTH, EModelVariant::TENT_CORNER}, {} };

    // 서버 전용 마커 (렌더링 없음)
    asset_table[EModelType::MONSTER_HUMAN] = { {}, {} };
    asset_table[EModelType::MONSTER_GHOST] = { {}, {} };
}

std::vector<std::string> CMapAssetManager::GetMeshNames(EModelType type, EModelVariant serverModelId)
{
    std::vector<std::string> results;

    // 메인 모델 결정
    if (serverModelId != EModelVariant::NONE) {
        // 서버에서 준 ID가 있으면 우선 사용
        if (id_to_file.contains(serverModelId)) {
            results.push_back(id_to_file[serverModelId]);
        }
    }
    else if (asset_table.contains(type)) {
        // 서버 ID가 없으면(싱글) main 전부 생성
        const auto& main_pool = asset_table[type].main_variants;
        if (!main_pool.empty()) {
            for (auto& main : main_pool) {
                results.push_back(id_to_file[main]);
            }
        }
        // 추가 장식물(Extra) 결정
        for (const auto& pool_key : asset_table[type].extra_pools) {
            if (random_pools.contains(pool_key)) {
                const auto& pool = random_pools[pool_key];
                results.push_back(pool[rand() % pool.size()]);
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
