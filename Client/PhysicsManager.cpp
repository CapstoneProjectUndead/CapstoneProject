#include "stdafx.h"
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"
#include "Movement.h"

void CPhysicsManager::Update(float deltaTime)
{
    colliders.erase(std::remove(colliders.begin(), colliders.end(), nullptr), colliders.end() );
}

bool CPhysicsManager::CheckFilter(const CollisionFilter& a, const CollisionFilter& b)
{
    // 서로의 마스크가 상대방의 카테고리를 포함하고 있는지 확인 (AND 연산)
    return (a.mask & b.category) && (b.mask & a.category);
}

void CPhysicsManager::BroadPhase(CColliderComponent* checkCol, const XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates)
{
    BoundingBox expanded{ checkCol->world_aabb };
    // 이동 경로의 중간 지점으로 센터 이동
    expanded.Center = Vector3::Add(expanded.Center, Vector3::ScalarProduct(delta, 0.5f));

    for (auto& col : colliders) {
        if (col.get() == checkCol) continue;
        // filtering. 물리적으로 충돌 설정이 되어 있는지 확인
        if (!CheckFilter(checkCol->filter, col->filter)) continue;

        if (expanded.Intersects(col->world_aabb))
            candidates.push_back(col.get());
    }
}

bool CPhysicsManager::Overlap(CObject* obj, const XMFLOAT3& delta, CollisionInfo& collisionInfo, uint32_t mask)
{
    auto* col = obj->GetComponent<CColliderComponent>();
    if (!col) return false;

    // 마스크가 0이면 컴포넌트 기본 마스크 사용, 아니면 지정 마스크 사용
    uint32_t originalMask = col->filter.mask;
    if (mask != 0) col->filter.mask = mask;

    std::vector<CColliderComponent*> candidates;
    BroadPhase(col, delta, candidates);

    bool bHit = false;
    for (auto* other : candidates) {
        auto supportA = [&](XMVECTOR d) { return col->shape->GetSupport(d); };
        auto supportB = [&](XMVECTOR d) { return other->shape->GetSupport(d); };

        GJKAlgorithm::Simplex simplex;
        if (GJKAlgorithm::GenericIntersects(supportA, supportB, simplex)) {
            collisionInfo = GJKAlgorithm::SolveEPA(simplex, col->shape.get(), other->shape.get());
            if (collisionInfo.collided) {
                bHit = true;
                collisionInfo.other_object = other->owner;
                break;
            }
        }
    }

    // 마스크 복구
    col->filter.mask = originalMask;
    return bHit;
}

bool CPhysicsManager::Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, CollisionInfo& outInfo)
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
        if (auto* meshShape = dynamic_cast<CConcaveMeshShape*>(other->shape.get())) {
            for (const auto& tri : meshShape->GetWorldTriangles()) {
                float hitDist = 0.0f;
                if (Triangle::Intersect(origin, direction, tri.v[0], tri.v[1], tri.v[2], hitDist)) {
                    // 지금까지 찾은 것보다 가깝다면
                    if (hitDist > 0 && hitDist <= closestDist) {
                        closestDist = hitDist;

                        // 법선 계산
                        XMFLOAT3 edge1 = Vector3::Subtract(tri.v[1], tri.v[0]);
                        XMFLOAT3 edge2 = Vector3::Subtract(tri.v[2], tri.v[0]);
                        outInfo.normal = XMLoadFloat3(&Vector3::CrossProduct(edge1, edge2));

                        outInfo.depth = maxDistance - hitDist;
                        bHit = true;
                    }
                }
            }
        }
    }

    return bHit;
}

void CPhysicsManager::ApplyGravity(CObject* obj, float dt)
{
    auto* col = obj->GetComponent<CColliderComponent>();
    if (!col) return;

    // 지면 체크 (아주 살짝 아래 방향으로 Overlap 체크)
    XMFLOAT3 downDelta = { 0, -0.1f, 0 };
    CollisionInfo info{};
    obj->is_grounded = Overlap(obj, downDelta, info, EColLayer::GROUND | EColLayer::OBJECT);

    if (obj->is_grounded) {
        // 바닥 위로 밀어올리기 (Y축 보정)
        if (XMVectorGetY(info.normal) < 0) info.normal = -info.normal;
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        // 법선과 하늘 방향의 내적으로 각도 계산 (cos theta)
        float cosTheta = XMVectorGetX(XMVector3Dot(info.normal, up));
        float slopeAngle = XMVectorGetX(XMVector3AngleBetweenVectors(info.normal, up));
        float maxSlopeAngle = XMConvertToRadians(45.0f); // 45도까지는 안 미끄러짐

        if (slopeAngle < maxSlopeAngle) {
            // 경사가 완만하면 Y축 속도를 완전히 죽이고 위치를 바닥에 고정
            if (obj->velocity.y <= 0) {
                obj->velocity.y = 0;

                // 바닥에 딱 붙이기 (Y축 보정)
                XMVECTOR separation = info.normal * info.depth;
                XMVECTOR curPos = XMLoadFloat3(&obj->position);
                XMStoreFloat3(&obj->position, curPos + separation);
            }
        }
        else {
            // 경사가 너무 가파르면 미끄러짐 처리 (Slide 적용)
            XMVECTOR v = XMLoadFloat3(&obj->velocity);
            XMVECTOR dot = XMVector3Dot(v, info.normal);
            XMVECTOR slideVel = v - info.normal * dot;
            XMStoreFloat3(&obj->velocity, slideVel);

            // 중력의 일부를 경사면 아래 방향으로 가함
            obj->velocity.y += gravity * dt * 0.5f;
        }

        // 지면 마찰력 적용
        if (obj->velocity.y < 0) obj->velocity.y = 0;

        float speedLen = Vector3::Length(obj->velocity);
        float decel = obj->friction * dt;
        if (decel > speedLen) decel = speedLen;

        obj->velocity = Vector3::Add(obj->velocity, Vector3::ScalarProduct(obj->velocity, -decel, true));
    }
    else {
        // 공중일 때: 중력 가속
        obj->velocity.y += gravity * dt;
    }
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