#include "stdafx.h"
#include "ItemFinder.h"
#include "KeyManager.h"
#include "MyPlayer.h"

CItemFinder::CItemFinder()
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
        float res = SearchNearbyTreasure(owner->position, 10.f);
        if (res == -1) {
            int a = 0;
        }
        else {
            int a = 0;
        }
    }
}

void CItemFinder::RegisterTreasures(const std::vector<MapGenerator::InstanceData>& mapData)
{
    treasure_positions.clear();

    // 맵 데이터를 순회하며 보물만 찾아 벡터에 저장
    for (const auto& instance : mapData) {
        if (instance.type == MapGenerator::EModelType::TREASURE) {
            treasure_positions.push_back(instance.position);
        }
    }
}

float CItemFinder::SearchNearbyTreasure(const XMFLOAT3& playerPos, float radius)
{
    float minDistance = -1.0f; // 발견 못 함을 의미

    for (const auto& treasurePos : treasure_positions) {

        // XZ 평면 거리만 구합니다. (높이 무시)
        XMFLOAT3 dirVec = {
            treasurePos.x - playerPos.x,
            0.0f,
            treasurePos.z - playerPos.z
        };

        float dist = sqrtf((dirVec.x * dirVec.x) + (dirVec.z * dirVec.z));

        // 탐색 반경(radius) 안에 들어왔다면?
        if (dist <= radius) {
            // 가장 가까운 보물의 거리를 갱신
            if (minDistance < 0.0f || dist < minDistance) {
                minDistance = dist;
            }
        }
    }

    return minDistance;
}
