#pragma once
#include "Component.h"
#include "GJKAlgorithm.h"

class CMesh;

class CColliderShape {
public:
    virtual ~CColliderShape() = default;

    virtual void Update(const XMMATRIX& worldMatrix) = 0;

    // GJK 특정 방향으로 가장 먼 월드 좌표 점 반환
    virtual XMVECTOR GetSupport(XMVECTOR direction) const = 0;

    // 디버그 렌더링용 (선택)
    virtual void Render() {}
protected:
    std::shared_ptr<CMesh> debug;
};

// obb 기반
class CBoxShape : public CColliderShape {
public:
    CBoxShape(XMFLOAT3 extents, XMFLOAT3& p = XMFLOAT3{}) {
        local.Center = p;
        local.Extents = extents;
    };

    void Render() override;
    void Update(const XMMATRIX& worldMatrix) override;
    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupportOBB(world, direction);
    }
private:
    BoundingOrientedBox local;
    BoundingOrientedBox world;
};

class CSphereShape : public CColliderShape {
public:
    CSphereShape(float r, XMFLOAT3& p = XMFLOAT3{}) {
        local.Radius = r;
        local.Center = p;
    }

    void Render() override;
    void Update(const XMMATRIX& worldMatrix) override;
    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupportSphere(world, direction);
    }
private:
    BoundingSphere local;
    BoundingSphere world;
};

class CConvexMeshShape : public CColliderShape
{
public:
    CConvexMeshShape(std::vector<XMFLOAT3>& vertice);

    void Update(const XMMATRIX& worldMatrix) override;

    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupport(world, direction);
    }
private:
    std::vector<XMFLOAT3> local;
    std::vector<XMFLOAT3> world;
};

/*
충돌 모양 데이터 제공자. 물리 계산X
* ColliderComponent 생성법
* paramaeter: shape(unique), bounds
* 
std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents);
auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
obj->SetComponent(boxCollider);
CPhysicsManager::GetInstance().SetCollider(boxCollider);
*/
class CColliderComponent : public CComponent
{
public:
    CColliderComponent(std::unique_ptr< CColliderShape>& otherShape, const BoundingBox& otherBox);
    void SetShape(std::unique_ptr< CColliderShape>& otherShape) { shape = std::move(otherShape); }

    void Update(const float deltaTime) override;
    // 디버깅용(aabb 출력)
    void Render(ID3D12GraphicsCommandList* commandList) override;

    bool Intersects(const CColliderComponent* other);
private:
    friend class CPhysicsManager;
    std::unique_ptr<CColliderShape> shape;
    BoundingBox local_aabb;
    BoundingBox world_aabb{};   // for broad phase
    std::shared_ptr<CMesh> debug;
};
