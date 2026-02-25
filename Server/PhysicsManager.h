#pragma once
// Server쪽 PhysicsManager

class CColliderComponent;
class CObject;
class CMovementComponent;

#include "GJKAlgorithm.h"

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

    void ApplyGravity(CObject* obj, float dt);

    // query
    bool Overlap(CObject* obj, XMFLOAT3& delta, GJKAlgorithm::CollisionInfo& collisionInfo);
    bool Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, GJKAlgorithm::CollisionInfo& outInfo);

    void Update(float deltaTime);

private:
    // 충돌 후보 추리기(자기 자신 제외)
    void BroadPhase(CColliderComponent* checkCol, XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates);

    // Compute
    XMFLOAT3 ComputeCollisionNormal(CColliderComponent* a, CColliderComponent* b);
    float ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal);
    XMFLOAT3 ComputeCollisionDistance(const BoundingBox& a, const BoundingBox& b);
    // 충돌 시간 계산(for Sweep())
    bool ComputeCollisionTime(CColliderComponent* a, CColliderComponent* b, const XMFLOAT3& delta, float& outTime);

private:
    std::vector<std::shared_ptr<CColliderComponent>> colliders;
    float gravity{-9.8f};
};