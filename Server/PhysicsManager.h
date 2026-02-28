#pragma once
// Server쪽 PhysicsManager

class CColliderComponent;
class CConcaveMeshShape;
class CObject;
class CMovementComponent;
struct CollisionInfo;
struct CollisionFilter;

#include "CollisionAlgorithm.h"

/*
충돌 감지 및 계산(캐릭터는 별도로 처리)
오브젝트 필요하면 멤버로 추가
*/

class CPhysicsManager
{
private:
    CPhysicsManager() {};
    CPhysicsManager(const CPhysicsManager&) = delete;
public:
    ~CPhysicsManager() {};

    static CPhysicsManager& GetInstance() {
        static CPhysicsManager instance;
        return instance;
    }

    void SetCollider(std::shared_ptr<CColliderComponent> c) {
        colliders.push_back(c);
    }

    XMVECTOR ApplyGravity(CObject* obj, float dt);
    void ApplyFriction(CObject* obj, float dt);
    bool CheckFilter(const CollisionFilter& a, const CollisionFilter& b);

    // query
    bool Overlap(CObject* obj, const XMFLOAT3& delta, CollisionInfo& collisionInfo, uint32_t mask = 0);
    bool Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, CollisionInfo& outInfo);

    void Update(float deltaTime);

private:
    // 충돌 후보 추리기(자기 자신 제외)
    void BroadPhase(CColliderComponent* checkCol, const XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates);
    // concave 충돌
    bool OverlapConcave(CConcaveMeshShape* concaveShape, CColliderComponent* convexCol, CollisionInfo& outInfo);

    // Compute
    XMFLOAT3 ComputeCollisionNormal(CColliderComponent* a, CColliderComponent* b);
    float ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal);
    XMFLOAT3 ComputeCollisionDistance(const BoundingBox& a, const BoundingBox& b);
    // 충돌 시간 계산(for Sweep())
    bool ComputeCollisionTime(CColliderComponent* a, CColliderComponent* b, const XMFLOAT3& delta, float& outTime);
private:
    std::vector<std::shared_ptr<CColliderComponent>> colliders;
    float gravity{ -9.8f };
};