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
    void ResolveCollision(CColliderComponent* a, CColliderComponent* b, float dt);
    float ComputePenetration(const BoundingBox& a, const BoundingBox& b, const XMFLOAT3& normal);

    void Update(float deltaTime);
    void ApplyMovement(float dt);
private:
    void BroadPhaseSAP(std::vector<std::pair<CColliderComponent*, CColliderComponent*>>& outPairs);

    std::vector<std::shared_ptr<CColliderComponent>> colliders;
};