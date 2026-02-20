#include "pch.h"
// Server쪽 PhysicsManager
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"


void CPhysicsManager::Update(float deltaTime)
{
    colliders.erase(std::remove(colliders.begin(), colliders.end(), nullptr), colliders.end() );
}

void CPhysicsManager::BroadPhase(CColliderComponent* checkCol, XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates)
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

bool CPhysicsManager::Overlap(CObject* obj, XMFLOAT3& delta, GJKAlgorithm::CollisionInfo& collisionInfo)
{
    auto* col = obj->GetComponent<CColliderComponent>();
    if (!col) return false;

    std::vector<CColliderComponent*> candidates;
    BroadPhase(col, delta, candidates);

    for (auto* other : candidates) {
        auto supportA = [&](XMVECTOR d) { return col->shape->GetSupport(d); };
        auto supportB = [&](XMVECTOR d) { return other->shape->GetSupport(d); };

        // GJK 실행 및 Simplex 획득
        GJKAlgorithm::Simplex simplex;
        if (GJKAlgorithm::GenericIntersects(supportA, supportB, simplex)) {
            // EPA 실행
            collisionInfo = GJKAlgorithm::SolveEPA(simplex, col->shape.get(), other->shape.get());
            if (collisionInfo.collided) return true;
        }
    }

    return false;
}

bool CPhysicsManager::Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, GJKAlgorithm::CollisionInfo& outInfo)
{
    XMVECTOR rayOrigin = XMLoadFloat3(&origin);
    XMVECTOR rayDir = XMVector3Normalize(XMLoadFloat3(&direction));

    bool bHit = false;
    float closestDist = maxDistance; // maxDistance 내의 가장 가까운 거리 찾기

    for (auto& other : colliders) {
        // Broad-phase: 레이와 콜라이더의 전체 AABB가 만나는지 먼저 체크
        float aabbDist = 0.0f;
        if (!other->world_aabb.Intersects(rayOrigin, rayDir, aabbDist)) continue;
        if (aabbDist > closestDist) continue;

        // Narrow-phase: 메쉬 콜라이더일 경우
        if (auto* meshShape = dynamic_cast<CTriangleMeshShape*>(other->shape.get())) {
            for (const auto& tri : meshShape->GetWorldTriangles()) {
                float hitDist = 0.0f;
                if (Triangle::Intersect(origin, direction, tri.v[0], tri.v[1], tri.v[2], hitDist)) {
                    // 지금까지 찾은 것보다 가깝다면
                    if (hitDist > 0 && hitDist <= closestDist) {
                        closestDist = hitDist;

                        // 법선 계산
                        XMFLOAT3 tmp = tri.v[0];
                        XMFLOAT3 tmp1 = tri.v[1];
                        XMFLOAT3 tmp2 = tri.v[2];

                        XMFLOAT3 edge1 = Vector3::Subtract(tmp1, tmp);
                        XMFLOAT3 edge2 = Vector3::Subtract(tmp2, tmp);

                        XMFLOAT3 result = Vector3::CrossProduct(edge1, edge2);
                        outInfo.normal = XMLoadFloat3(&result);

                        outInfo.depth = maxDistance - hitDist;
                        bHit = true;
                    }
                }
            }
        }
    }

    return bHit;
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
    XMFLOAT3 result = ComputeCollisionDistance(a->world_aabb, b->world_aabb);
    return Vector3::Normalize(result);
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
    XMFLOAT3 tmp = Vector3::Subtract(minB, maxA);
    XMFLOAT3 tmp2 = Vector3::Subtract(maxB, minA);
    XMFLOAT3 t1 = Vector3::Multiply(tmp, invDelta);
    XMFLOAT3 t2 = Vector3::Multiply(tmp2, invDelta);

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