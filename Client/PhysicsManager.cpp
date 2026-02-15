#include "stdafx.h"
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"
#include "Movement.h"

void CPhysicsManager::BroadPhaseSAP(CColliderComponent* checkCol, const XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates)
{
    BoundingBox expanded{ checkCol->world_aabb };

    // 이동 경로의 중간 지점으로 센터 이동
    expanded.Center = Vector3::Add(expanded.Center, Vector3::ScalarProduct(delta, 0.5f));

    for (auto& col : colliders) {
        if (col.get() == checkCol) continue;
        if (expanded.Intersects(col->world_aabb))
            candidates.push_back(col.get());
    }
}

bool CPhysicsManager::Sweep(CObject* obj, const XMFLOAT3& delta, SweepHit& outHit)
{
    auto* col = obj->GetComponent<CColliderComponent>();
    if (!col) return false;

    // 1) SAP로 충돌 후보 추리기
    std::vector<CColliderComponent*> candidates;
    BroadPhaseSAP(col, delta, candidates);

    CColliderComponent* hitCol{};
    float earliest{ 1.0f };

    for (auto* other : candidates) {
        float t{};

        if (!ComputeCollisionTime(col, other, delta, t)) {
            continue;
        }
        if (!col->Intersects(other))
            continue;

        if (t < earliest) {
            earliest = t;
            hitCol = other;
        }
    }

    if (hitCol) {
        outHit.other = hitCol;
        outHit.normal = ComputeCollisionNormal(col, hitCol);
        outHit.time = earliest;
        return true;
    }

    return false;
}

bool CPhysicsManager::CheckGround(CColliderComponent* col)
{
    const auto& aabb = col->world_aabb;

    for (auto& other : colliders) {
        if (other.get() == col) continue;

        const auto& b = other->world_aabb;

        // 바닥 판정: AABB bottom이 다른 AABB top보다 아래로 내려갔는가?
        if (aabb.Center.y - aabb.Extents.y <= b.Center.y + b.Extents.y + 0.05f) {
            // XZ가 겹쳐야 진짜 바닥
            bool overlapX = fabs(aabb.Center.x - b.Center.x) <= (aabb.Extents.x + b.Extents.x);
            bool overlapZ = fabs(aabb.Center.z - b.Center.z) <= (aabb.Extents.z + b.Extents.z);

            if (overlapX && overlapZ)
                return true;
        }
    }

    return false;
}

void CPhysicsManager::ApplyGravity(CObject* obj, float dt)
{
    // 1) 지면 체크
    auto* col = obj->GetComponent<CColliderComponent>();
    if (col)
        obj->is_grounded = CheckGround(col);
    else
        obj->is_grounded = false;

    // 2) 중력 적용
    if (!obj->is_grounded)
        obj->velocity.y += gravity * dt;
    else
        obj->velocity.y = 0;

    // 감속(마찰)
    float speedLen = Vector3::Length(obj->velocity);
    float decel = obj->friction * dt;
    if (decel > speedLen) decel = speedLen;

    obj->velocity = Vector3::Add(obj->velocity, Vector3::ScalarProduct(obj->velocity, -decel, true));
}

XMFLOAT3 CPhysicsManager::ComputeCollisionNormal(CColliderComponent* a, CColliderComponent* b)
{
    return Vector3::Normalize(ComputeCollisionDistance(a->world_aabb, b->world_aabb));
}

float CPhysicsManager::ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal)
{
    XMFLOAT3 d{ ComputeCollisionDistance(a, b) };

    // normal 방향으로 가장 작은 penetration을 선택
    float pen = 0.0f;

    if (fabsf(normal.x) > 0.5f) pen = d.x;
    else if (fabsf(normal.y) > 0.5f) pen = d.y;
    else pen = d.z;

    return pen;
}

XMFLOAT3 CPhysicsManager::ComputeCollisionDistance(const BoundingBox& a, const BoundingBox& b)
{
    float dx = (a.Extents.x + b.Extents.x) - fabsf(a.Center.x - b.Center.x);
    float dy = (a.Extents.y + b.Extents.y) - fabsf(a.Center.y - b.Center.y);
    float dz = (a.Extents.z + b.Extents.z) - fabsf(a.Center.z - b.Center.z);

    return XMFLOAT3{ dx, dy, dz };
}

bool CPhysicsManager::ComputeCollisionTime(CColliderComponent* colA, CColliderComponent* colB, const XMFLOAT3& delta, float& outTime)
{
    // delta가 0이면 sweep 의미 없음
    if (delta.x == 0 && delta.y == 0 && delta.z == 0)
        return false;

    BoundingBox a{ colA->world_aabb };
    BoundingBox b{ colB->world_aabb };

    // AABB min/max
    XMFLOAT3 minA = Vector3::Subtract(a.Center, a.Extents);
    XMFLOAT3 maxA = Vector3::Add(a.Center, a.Extents);

    XMFLOAT3 minB = Vector3::Subtract(b.Center, b.Extents);
    XMFLOAT3 maxB = Vector3::Add(b.Center, b.Extents);

    // delta의 역수
    XMFLOAT3 invDelta = {
        delta.x != 0 ? 1.0f / delta.x : FLT_MAX,
        delta.y != 0 ? 1.0f / delta.y : FLT_MAX,
        delta.z != 0 ? 1.0f / delta.z : FLT_MAX
    };

    // t1(A-B), t2(B-A) 계산(닿는 순간)
    XMFLOAT3 t1 = Vector3::Multiply(Vector3::Subtract(minB, maxA), invDelta);
    XMFLOAT3 t2 = Vector3::Multiply(Vector3::Subtract(maxB, minA), invDelta);

    XMVECTOR vt1 = XMLoadFloat3(&t1);
    XMVECTOR vt2 = XMLoadFloat3(&t2);

    // 처음 닿고 빠져나가는 시간(A, B 상관 없이 먼저)
    XMVECTOR minVec = XMVectorMin(vt1, vt2); // 결과: (1.0f, 2.0f, 3.0f)
    XMVECTOR maxVec = XMVectorMax(vt1, vt2);

    // 각 축에서 먼저 닿고 빠져나가는 시간(충돌 시간)
    float tEntry = max(XMVectorGetX(minVec), max(XMVectorGetY(minVec), XMVectorGetZ(minVec)));
    float tExit = min(XMVectorGetX(maxVec), min(XMVectorGetY(maxVec), XMVectorGetZ(maxVec)));

    // 충돌 조건
    if (tEntry > tExit) return false;   // 축별 충돌 시간이 겹치지 않음
    if (tExit < 0.0f) return false;     // 충돌이 과거에 발생
    if (tEntry > 1.0f) return false;    // delta 범위 밖에서 충돌

    outTime = tEntry;

    return true;
}