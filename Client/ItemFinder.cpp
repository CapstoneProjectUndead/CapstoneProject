#include "stdafx.h"
#include "ItemFinder.h"
#include "KeyManager.h"
#include "MyPlayer.h"
#include "UIComponent.h"

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
    float res = SearchNearbyTreasure(owner->position);
    auto ui = owner->GetComponent<CUIDowsingArrow>();

    if (res == -1) {
        int a = 0;

        if (ui) {
            ui->SetEnable(false);
        }
    }
    else {
        // player 위치로 투영
        XMFLOAT3 dirWorld = Vector3::Subtract(closest_treasure_pos, owner->GetPosition());
        float dx = Vector3::DotProduct(dirWorld, owner->right);
        float dz = Vector3::DotProduct(dirWorld, owner->look);
        angle = atan2f(dx, dz);

        if (ui) {
            ui->SetEnable(true);
        }
#ifdef DEBUG
        std::cout << "Player pos: " << owner->GetPosition().x << ", " 
            << owner->GetPosition().z << std::endl;
           
        std::cout << "Treasure pos: " << closest_treasure_pos.x << ", " 
            << closest_treasure_pos.z << std::endl;
#endif // DEBUG
    }
}

void CItemFinder::RegisterTreasures(const std::vector<MapGenerator::InstanceData>& mapData)
{
    treasures.clear();

    // 맵 데이터를 순회하며 보물만 찾아 벡터에 저장 (땅속 보물 포함)
    for (const auto& instance : mapData) {
        if (instance.type == MapGenerator::EModelType::TREASURE || instance.type == MapGenerator::EModelType::TREASURE_HIDDEN || 
            instance.type == MapGenerator::EModelType::TREASURE_VILLAGE) {
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

void CItemFinder::Toggle()
{
    SetEnable(!is_enable);

    auto ui = owner->GetComponent<CUIDowsingArrow>();
    if (ui) {
        ui->SetEnable(is_enable);
    }
}
