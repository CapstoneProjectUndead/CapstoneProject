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
public:
    CPhysicsManager();
    ~CPhysicsManager();
    CPhysicsManager(const CPhysicsManager&) = delete;

public:
    void SetCollider(CColliderComponent* c) {
        colliders.push_back(c);
    }

    void SetCollider(std::shared_ptr<CColliderComponent> c) {
        colliders.push_back(c.get());
    }

    void ClearCollider() { colliders.clear(); }

    XMVECTOR ApplyGravity(CObject* obj, float dt);
    void ApplyFriction(CObject* obj, float dt);
    bool CheckFilter(const CollisionFilter& a, const CollisionFilter& b);

    // query
    bool Overlap(CObject* obj, const XMFLOAT3& delta, CollisionInfo& collisionInfo, uint32_t mask = 0);
    bool Raycast(const XMFLOAT3& origin, const XMFLOAT3& direction, float maxDistance, CollisionInfo& outInfo);

    void Update(float deltaTime);

    void EraseCollider(OBJECT_TYPE objType, SCENE_TYPE sceneType);
    void EraseCollider(CColliderComponent* coll);

    // owner 객체가 가진 모든 collider의 충돌 mask를 일괄 변경
    // (한 객체에 mesh/box/sphere/capsule 등 여러 collider가 등록될 수 있어
    //  GetComponent<CColliderComponent>()로는 첫 번째만 잡힘 → 이 함수로 전체 처리)
    void SetMaskByOwner(CObject* owner, uint32_t newMask);

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
    std::vector<CColliderComponent*> colliders;
    float gravity{ -9.8f };
};