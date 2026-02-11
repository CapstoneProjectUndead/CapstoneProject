#pragma once
#include "Component.h"

class CMesh;

class CColliderShape {
public:
    virtual ~CColliderShape() = default;

    virtual void Update(const XMMATRIX& worldMatrix) = 0;

    // broad phase용 AABB 계산
    virtual void ComputeAABB(BoundingBox& outAABB) const = 0;

    virtual const BoundingOrientedBox* GetOBB() const { return nullptr; }
    virtual const BoundingSphere* GetSphere() const { return nullptr; }

    // 디버그 렌더링용 (선택)
    virtual void Render() const {}
};

// obb 기반
class CBoxShape : public CColliderShape {
public:
    CBoxShape(XMFLOAT3 extents) : base_extents{extents} {};

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override;
    const BoundingOrientedBox* GetOBB() const override { return &obb; }
private:
    BoundingOrientedBox obb;
    XMFLOAT3 base_extents;
};

class CSphereShape : public CColliderShape {
public:
    CSphereShape(float r) : radius(r) {
        sphere.Radius = r;
    }

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override;
    const BoundingSphere* GetSphere() const override { return &sphere; }
private:
    BoundingSphere sphere;
    float radius;
};

class CCapsuleShape : public CColliderShape {
public:
    CCapsuleShape(float r, float h) 
        : radius(r), height(h), base_radius{r} { }

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override;
private:
    float radius;
    float base_radius;
    float height;

    // 월드 공간에서의 캡슐 중심선
    XMFLOAT3 top;
    XMFLOAT3 bottom;
};

// 충돌 모양 데이터 제공자. 물리 계산X
class CColliderComponent : public CComponent
{
public:
    CColliderComponent(std::unique_ptr< CColliderShape>& otherShape) : shape{ std::move(otherShape) } {}
    void SetShape(std::unique_ptr< CColliderShape>& otherShape) { shape = std::move(otherShape); }

    void Update(const float deltaTime) override;

    bool Intersects(const CColliderComponent* other);

    const BoundingBox& GetAABB() const { return aabb; }
private:
    friend class CPhysicsManager;
    std::unique_ptr<CColliderShape> shape;
    BoundingBox aabb;   // for broad phase
};
