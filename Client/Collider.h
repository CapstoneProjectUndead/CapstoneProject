#pragma once
#include "Component.h"

class CMesh;

enum class EColliderType
{
    Box,
    Sphere,
    Capsule
};

// 충돌 모양 데이터 제공자. 물리 계산X
class CColliderComponent : public CComponent
{
public:
    virtual void Update(const float deltaTime) override {}

    virtual BoundingBox GetBounds() const = 0;

    bool Intersect(CColliderComponent* other) { return world_bounds.Intersects(other->world_bounds); }
protected:
    bool is_trigger{ false };
    EColliderType type{};

    BoundingBox local_bounds;   // 모델 기준
    BoundingBox world_bounds;
    friend class CPhysicsManager;
};

class CBoxColliderComponent : public CColliderComponent
{
public:
    CBoxColliderComponent() { type = EColliderType::Box; }

    void Update(float deltaTime) override;

    void SetLocalBounds(const BoundingBox& bounds);
    BoundingBox GetBounds() const override { return world_bounds; }
};