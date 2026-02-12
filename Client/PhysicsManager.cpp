#include "stdafx.h"
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"
#include "Movement.h"

void CPhysicsManager::Update(float dt)
{
    // 1) SAP broad-phase
    std::vector<std::pair<CColliderComponent*, CColliderComponent*>> pairs;
    BroadPhaseSAP(pairs);

    // 2) Narrow-phase 충돌 처리
    for (auto& p : pairs) {
        auto* a = p.first;
        auto* b = p.second;

        // Shape 기반 narrow-phase
        if (!a->Intersects(b))
            continue;

        ApplyCollision(a, b, dt);
    }

    // 3) 위치 업데이트
    ApplyMovement(dt);
}

void CPhysicsManager::BroadPhaseSAP(std::vector<std::pair<CColliderComponent*, CColliderComponent*>>& outPairs)
{
    std::vector<EndPoint> endpoints;
    endpoints.reserve(colliders.size() * 2);

    // X Axis
    // 왼쪽 오른쪽 끝 점
    for (auto col : colliders) {
        const auto& box = col->aabb;
        endpoints.push_back(EndPoint{ box.Center.x - box.Extents.x, col.get(), true});
        endpoints.push_back(EndPoint{ box.Center.x + box.Extents.x, col.get(), false });
    }

    // 정렬
    std::sort(endpoints.begin(), endpoints.end(),
        [](const EndPoint& a, const EndPoint& b) {
            return a.value < b.value;
        });

    // 충돌 예상 시 push
    std::vector<CColliderComponent*> active;

    for (auto& ep : endpoints) {
        if (ep.is_min) {
            for (auto other : active)
                outPairs.push_back({ ep.col, other });

            active.push_back(ep.col);
        }
        else {
            active.erase(std::remove(active.begin(), active.end(), ep.col), active.end());
        }
    }
}

void CPhysicsManager::ApplyMovement(float dt)
{
    for (auto& col : colliders)
    {
        CObject* obj = col->owner;
        CMovementComponent* move = obj->GetComponent<CMovementComponent>();

        if (!move) continue;

        XMFLOAT3 moveVec = move->desired_move;

        moveVec = Vector3::ScalarProduct(obj->GetVelocity(), dt);

        // 실제 이동 적용
        obj->position = Vector3::Add(obj->position, moveVec);
    }
}

void CPhysicsManager::ApplyCollision(CColliderComponent* a, CColliderComponent* b, float dt)
{
    CObject* objA = a->owner;
    CObject* objB = b->owner;

    // 충돌 normal 계산 (A → B 방향)
    XMFLOAT3 normal = {
        objA->position.x - objB->position.x,
        objA->position.y - objB->position.y,
        objA->position.z - objB->position.z
    };
    normal = Vector3::Normalize(normal);
    //XMStoreFloat3(&normal, n);

    // penetration 계산
    float penetration = ComputePenetration(a->GetAABB(), b->GetAABB(), normal);

    // correction = normal * penetration * 0.5
    XMFLOAT3 corr = Vector3::ScalarProduct(normal, (penetration * 0.5f));

    // MovementComponent 슬라이딩 처리
    if (auto moveA = objA->GetComponent<CMovementComponent>())
        moveA->Slide(corr);

    if (auto moveB = objB->GetComponent<CMovementComponent>())
        moveB->Slide({ Vector3::ScalarProduct(corr, -1) });
}

float CPhysicsManager::ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal)
{
    float dx = (a.Extents.x + b.Extents.x) - fabsf(a.Center.x - b.Center.x);
    float dy = (a.Extents.y + b.Extents.y) - fabsf(a.Center.y - b.Center.y);
    float dz = (a.Extents.z + b.Extents.z) - fabsf(a.Center.z - b.Center.z);

    // normal 방향으로 가장 작은 penetration을 선택
    float pen = 0.0f;

    if (fabsf(normal.x) > 0.5f) pen = dx;
    else if (fabsf(normal.y) > 0.5f) pen = dy;
    else pen = dz;

    return pen;
}