#include "stdafx.h"
#include "ItemFinder.h"
#include "KeyManager.h"
#include "MyPlayer.h"

CItemFinder::CItemFinder()
    : closest_treasure_pos{}
{
}

CItemFinder::~CItemFinder()
{
}

void CItemFinder::Initialize()
{
}

void CItemFinder::Update(const float deltaTime)
{
    if (KEY_PRESSED(KEY::F)) {
        float res = SearchNearbyTreasure(owner->position);
        if (res == -1) {
            int a = 0;
        }
        else {
            std::cout << "Player pos: " << owner->GetPosition().x << ", " 
                << owner->GetPosition().z << std::endl;
           
            std::cout << "Treasure pos: " << closest_treasure_pos.x << ", " 
                << closest_treasure_pos.z << std::endl;
        }
    }
}

void CItemFinder::RegisterTreasures(const std::vector<MapGenerator::InstanceData>& mapData)
{
    treasures.clear();

    // 맵 데이터를 순회하며 보물만 찾아 벡터에 저장
    for (const auto& instance : mapData) {
        if (instance.type == MapGenerator::EModelType::TREASURE) {
            treasures.emplace_back(TreasureInfo{ instance.position });
        }
    }
}

void CItemFinder::RegisterTreasures(const std::vector<TreasureInfo>& _treasures)
{
    treasures.clear();
    treasures = _treasures;
}

float CItemFinder::SearchNearbyTreasure(const XMFLOAT3& playerPos)
{
    float minDistance = -1.0f; // 발견 못 함을 의미

    for (const auto& treasure : treasures) {

        // 유효한 상태(Vaild)인 보물만 검사
        // 이미 누가 파고 있거나(Occupied), 사라진(Invalid) 보물은 감지되지 않는다.
        if (treasure.treasure_state != TREASURE_STATE::Vaild) 
            continue;

        // XZ 평면 거리 계산
        float dx = treasure.treasure_pos.x - playerPos.x;
        float dz = treasure.treasure_pos.z - playerPos.z;
        float distSq = (dx * dx) + (dz * dz);
        float radiusSq = recog_range * recog_range;

        if (distSq <= radiusSq) {
            float dist = sqrtf(distSq);
            if (minDistance < 0.0f || dist < minDistance) {
                minDistance = dist;
                closest_treasure_pos = treasure.treasure_pos;
            }
        }
    }

    return minDistance;
}
