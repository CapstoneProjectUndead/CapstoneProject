#pragma once

class CColliderComponent;
class CObject;
class CMovementComponent;

struct SweepHit
{
    CColliderComponent* other;
    XMFLOAT3 normal;
    float time{ 1.0f };
};

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

    bool CheckGround(CColliderComponent* col);
    void ApplyGravity(CObject* obj, float dt);

    // query
    bool Sweep(CObject* obj, const XMFLOAT3& delta, SweepHit& outHit);

    void Update(float deltaTime) {};
private:
    // 자기 자신 제외
    void BroadPhaseSAP(CColliderComponent* checkCol, const XMFLOAT3& delta, std::vector<CColliderComponent*>& candidates);

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