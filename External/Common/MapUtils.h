#pragma once
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

#include "MapGenerator/MapGenerator.h"

struct ModelInfo {
    std::vector<EModelVariant> main_variants; // 메인 모델 (vending_machine001 등)
    std::vector<std::string> extra_pools;   // 함께 생성될 추가 풀 키 (grass, stone 등)
};

class CMapAssetManager {
public:
    static CMapAssetManager& GetInstance() {
        static CMapAssetManager instance;
        return instance;
    }

    void initialize();

    // 특정 모델 타입에 대해 최종적으로 생성할 메쉬 리스트 반환 (랜덤성 포함)
    std::vector<std::string> GetMeshNames(MapGenerator::EModelType type, EModelVariant serverModelId = EModelVariant::NONE);

    // no_collider_set에 없으면 false
    bool RequiresCollider(const std::string& meshName);
    EModelVariant GetVariantFromName(const std::string& meshName);
private:
    CMapAssetManager() = default;
    CMapAssetManager(const CMapAssetManager&) = delete;

    // EModelVariant -> 파일명 매핑
    std::unordered_map<EModelVariant, std::string> id_to_file;
    // 모델 타입별 매핑 정보 (랜덤 풀 포함)
    std::unordered_map<MapGenerator::EModelType, ModelInfo> asset_table;
    // 추가 소품용 랜덤 풀 (파일명 기반) - 클라에서만 사용
    std::unordered_map<std::string, std::vector<std::string>> random_pools;
    // 콜라이더 제외 목록 (grass, stone 등)
    std::unordered_set<std::string> no_collider_set;
};