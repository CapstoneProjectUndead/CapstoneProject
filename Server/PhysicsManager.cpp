#include "pch.h"
// Server쪽 PhysicsManager
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"


CPhysicsManager::CPhysicsManager()
{

}

CPhysicsManager::~CPhysicsManager()
{

}

void CPhysicsManager::Update(float deltaTime)
{
    colliders.erase(std::remove(colliders.begin(), colliders.end(), nullptr), colliders.end());
}

void CPhysicsManager::EraseCollider(OBJECT_TYPE objType, SCENE_TYPE sceneType)
{
    colliders.erase(std::remove_if(colliders.begin(), colliders.end(), [objType, sceneType](const CColliderComponent* coll) {
        return (coll->owner->obj_type == objType) && (coll->owner->current_scene_type == sceneType);
        }),
        colliders.end());
}

void CPhysicsManager::EraseCollider(CColliderComponent* coll)
{
    auto iter = std::find(colliders.begin(), colliders.end(), coll);

    if (iter != colliders.end()) {
        colliders.erase(iter);
    }
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
        if (col == checkCol) continue;
        // filtering. 물리적으로 충돌 설정이 되어 있는지 확인
        if (!CheckFilter(checkCol->filter, col->filter)) continue;

        if (expanded.Intersects(col->world_aabb))
            candidates.push_back(col);
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

    // 마스크 복구
    col->filter.mask = originalMask;

    for (auto* other : candidates) {
        if (auto* mesh = dynamic_cast<CConcaveMeshShape*>(other->shape.get())) {
            // 메쉬 내부에서 충돌하는 삼각형들을 찾아 EPA까지 돌려온다.
            CollisionInfo meshInfo;
            if (OverlapConcave(mesh, col, meshInfo)) {
                meshInfo.other_object = other->owner;
                collisionInfo = meshInfo;
                return true;
            }
        }
        else {
            GJKAlgorithm::Simplex simplex;
            auto supportA = [&](XMVECTOR d) { return col->shape->GetSupport(d); };
            auto supportB = [&](XMVECTOR d) { return other->shape->GetSupport(d); };
            if (GJKAlgorithm::GenericIntersects(supportA, supportB, simplex)) {
                collisionInfo = GJKAlgorithm::SolveEPA(simplex, col->shape.get(), other->shape.get());
                if (collisionInfo.collided) {
                    collisionInfo.other_object = other->owner;
                    return true;
                }
            }
        }

    }

    return false;
}

bool CPhysicsManager::OverlapConcave(CConcaveMeshShape* concaveShape, CColliderComponent* convexCol, CollisionInfo& outInfo)
{
    BoundingBox convexAABB = convexCol->GetWorldAABB();
    CColliderShape* convexShape = convexCol->GetShape();

    // BroadPhase
    // BVH를 통해 충돌 가능성이 있는 삼각형 인덱스만 빠르게 수집
    std::vector<int> candidateIndices;
    concaveShape->GetCandidateTrianglesBVH(convexAABB, candidateIndices);

    float maxDepth = -FLT_MAX;
    bool bHit = false;

    // index에 대해서만 GJK/EPA 수행
    for (int idx : candidateIndices) {
        const auto& tri = concaveShape->GetWorldTriangles()[idx];

        CTriangleShape triShape;
        triShape.v[0] = XMLoadFloat3(&tri.v[0]);
        triShape.v[1] = XMLoadFloat3(&tri.v[1]);
        triShape.v[2] = XMLoadFloat3(&tri.v[2]);

        GJKAlgorithm::Simplex simplex;
        auto supportA = [&](XMVECTOR d) { return convexShape->GetSupport(d); };
        auto supportB = [&](XMVECTOR d) { return triShape.GetSupport(d); };

        if (GJKAlgorithm::GenericIntersects(supportA, supportB, simplex)) {
            CollisionInfo info = GJKAlgorithm::SolveEPA(simplex, convexShape, &triShape);
            if (info.collided && info.depth > maxDepth) {
                maxDepth = info.depth;
                outInfo = info;
                bHit = true;
            }
        }
    }
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

XMVECTOR CPhysicsManager::ApplyGravity(CObject* obj, float dt)
{
    auto* col = obj->GetComponent<CColliderComponent>();
    if (!col) return XMVectorZero();;

    // 지면 체크 (아주 살짝 아래 방향으로 Overlap 체크)
    XMFLOAT3 downDelta = { 0, -0.1f, 0 };
    CollisionInfo info{};
    obj->is_grounded = Overlap(obj, downDelta, info, EColLayer::GROUND | EColLayer::OBJECT);

    XMVECTOR verticalSeparation = XMVectorZero();

    if (obj->is_grounded) {
        // 수직 속도 초기화 (땅에 박히는 것 방지)
        if (obj->velocity.y < 0) obj->velocity.y = 0;

        // 경사각 계산
        if (XMVectorGetY(info.normal) < 0) info.normal = -info.normal;
        XMVECTOR up = XMVectorSet(0, 1, 0, 0);
        // 법선과 하늘 방향의 내적으로 각도 계산 (cos theta)
        float slopeAngle = XMVectorGetX(XMVector3AngleBetweenVectors(info.normal, up));
        float maxSlopeAngle = XMConvertToRadians(45.0f); // 45도까지는 안 미끄러짐

        if (slopeAngle < maxSlopeAngle) {
            // 완만한 경사
            verticalSeparation = info.normal * info.depth;
        }
        else {
            // 가파른 경사 -> 미끄러짐 속도 적용
            XMVECTOR v = XMLoadFloat3(&obj->velocity);
            XMVECTOR slideVel = v - info.normal * XMVector3Dot(v, info.normal);
            XMStoreFloat3(&obj->velocity, slideVel);
            obj->velocity.y += gravity * dt * 0.5f;
        }

        // 지면 마찰력 적용
        ApplyFriction(obj, dt);
    }
    else {
        // 공중일 때: 중력 가속
        obj->velocity.y += gravity * dt;
    }
    return verticalSeparation;
}

void CPhysicsManager::ApplyFriction(CObject* obj, float dt)
{
    float speedLen = Vector3::Length(obj->velocity);
    if (speedLen > 0.0001f) {
        float decel = obj->friction * dt;
        if (decel > speedLen) decel = speedLen;

        XMVECTOR v = XMLoadFloat3(&obj->velocity);
        XMVECTOR frictionImpulse = XMVector3Normalize(v) * -decel;
        XMStoreFloat3(&obj->velocity, v + frictionImpulse);
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