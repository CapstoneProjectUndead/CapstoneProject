#pragma once

class CColliderComponent;
class CObject;
class CMovementComponent;

struct EndPoint {
    float value;   // minX 또는 maxX
    CColliderComponent* col;
    bool is_min;    // true = min, false = max
};

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
    void ApplyCollision(CColliderComponent* a, CColliderComponent* b, float dt);
    float ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal);
    bool CheckGround(CColliderComponent* col);
    void ApplyGravity(float dt);

    void Update(float deltaTime);
    void ApplyMovement(float dt);

    // 특정 충돌체 하나만 물리 연산(충돌 해결 -> 이동)을 수행하는 함수
    void SimulateSingleObject(CColliderComponent* col, float dt);

private:
    void BroadPhaseSAP(std::vector<std::pair<CColliderComponent*, CColliderComponent*>>& outPairs);

    std::vector<std::shared_ptr<CColliderComponent>> colliders;
    float gravity{-9.8f};
};