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

    // 우선 y축만 설정
    XMFLOAT3 pivot{};
};

// obb 기반
class CBoxShape : public CColliderShape {
public:
    CBoxShape(XMFLOAT3 extents, XMFLOAT3& p = XMFLOAT3{}) : base_extents{ extents } {
        pivot = p;
    };

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override;
    const BoundingOrientedBox* GetOBB() const override { return &obb; }
private:
    BoundingOrientedBox obb;
    XMFLOAT3 base_extents;
};

class CSphereShape : public CColliderShape {
public:
    CSphereShape(float r, XMFLOAT3& p = XMFLOAT3{}) : radius(r) {
        sphere.Radius = r;
        pivot = p;
    }

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override;
    const BoundingSphere* GetSphere() const override { return &sphere; }
private:
    BoundingSphere sphere;
    float radius;
};

class CConvexMeshShape : public CColliderShape
{
public:
    CConvexMeshShape(std::vector<XMFLOAT3>& vertice);
    std::vector<XMFLOAT3> localVertices;   // 로컬 공간 정점
    std::vector<XMFLOAT3> worldVertices;   // 월드 변환된 정점

    void Update(const XMMATRIX& worldMatrix) override;
    void ComputeAABB(BoundingBox& outAABB) const override {};

    // 충돌 검사 (SAT 기반)
    bool Intersects(const CConvexMeshShape& other) const;
};

/*
충돌 모양 데이터 제공자. 물리 계산X
* ColliderComponent 생성법
* 원하는 shape을 인자로 넘겨주면 됨
std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents);
auto boxCollider = std::make_shared<CColliderComponent>(shape);
obj->SetComponent(boxCollider);
CPhysicsManager::GetInstance().SetCollider(boxCollider);
*/
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