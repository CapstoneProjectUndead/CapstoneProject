#pragma once

class CColliderComponent;
class CObject;
class CMovementComponent;

struct EndPoint {
    float value;   // minX 또는 maxX
    CColliderComponent* col;        // collider index
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

    void Update(float deltaTime);
    void Move(CColliderComponent* collider, CMovementComponent* moveCom, const XMFLOAT3& delta);
    XMFLOAT3 ComputeCollisionNormal(CColliderComponent* a, CColliderComponent* b);
private:
    void BroadPhaseSAP(std::vector<std::pair<CColliderComponent*, CColliderComponent*>>& outPairs);

    std::vector<std::shared_ptr<CColliderComponent>> colliders;
};