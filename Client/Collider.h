#pragma once
#include "Component.h"
#include "CollisionAlgorithm.h"

class CMesh;
class CBoxShape;
class CSphereShape;

class CColliderShape {
public:
    CColliderShape() = default;
    virtual ~CColliderShape() = default;
    virtual std::unique_ptr<CColliderShape> Clone() const = 0;

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
    CBoxShape(XMFLOAT3 extents, XMFLOAT3& p = XMFLOAT3{});
    std::unique_ptr<CColliderShape> CBoxShape::Clone() const {
        return std::make_unique<CBoxShape>(*this);
    }

    void Render() override;
    void Update(const XMMATRIX& worldMatrix) override;
    // getter
    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupportOBB(world, direction);
    }
    BoundingOrientedBox& GetWorld() { return world; }
private:
    BoundingOrientedBox local{};
    BoundingOrientedBox world{};
};

class CSphereShape : public CColliderShape {
public:
    CSphereShape(float r, XMFLOAT3& p = XMFLOAT3{});
    CSphereShape(XMFLOAT3& extents, XMFLOAT3& p = XMFLOAT3{});
    std::unique_ptr<CColliderShape> CSphereShape::Clone() const {
        return std::make_unique<CSphereShape>(*this);
    }

    void Render() override;
    void Update(const XMMATRIX& worldMatrix) override;
    // getter
    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupportSphere(world, direction);
    }
    BoundingSphere& GetWorld() { return world; }
private:
    BoundingSphere local{};
    BoundingSphere world{};
};

/*
* 속이 찬 오브젝트(볼록)에 사용
*/
class CConvexMeshShape : public CColliderShape
{
public:
    CConvexMeshShape(std::vector<XMFLOAT3>& vertice);
    std::unique_ptr<CColliderShape> CConvexMeshShape::Clone() const {
        return std::make_unique<CConvexMeshShape>(*this);
    }
    
    void Update(const XMMATRIX& worldMatrix) override;

    XMVECTOR GetSupport(XMVECTOR direction) const override {
        return GJKAlgorithm::GetSupport(world, direction);
    }
    std::vector<XMFLOAT3>& GetWorld() { return world; }
private:
    std::vector<XMFLOAT3> local{};
    std::vector<XMFLOAT3> world{};
};

// concave를 GJK로 연산하기 위한 class
class CTriangleShape : public CColliderShape {
public:
    CTriangleShape() = default;
    std::unique_ptr<CColliderShape> CTriangleShape::Clone() const {
        return std::make_unique<CTriangleShape>(*this);
    }

    XMVECTOR v[3];

    XMVECTOR GetSupport(XMVECTOR direction) const override;

    // 삼각형은 단순 점들의 집합이므로 별도의 업데이트 불필요 (이미 월드 좌표임)
    virtual void Update(const XMMATRIX& worldMatrix) override {}
};

/*
* 오목한 물체에 사용(특히 지형)
* BVH 기법으로 빠르게 연산
*/
class CConcaveMeshShape : public CColliderShape {
public:
    struct BVHNode {
        BoundingBox aabb;
        int left = -1;      // 왼쪽 자식 인덱스
        int right = -1;     // 오른쪽 자식 인덱스
        int triangle_idx = -1; // -1이 아니면 이 노드는 '잎(Leaf)'이며 실제 삼각형을 가리킴

        bool IsLeaf() const { return triangle_idx != -1; }
    };

    struct Triangle {
        std::array<XMFLOAT3, 3> v;
        BoundingBox aabb;
    };
    CConcaveMeshShape(const std::vector<XMFLOAT3>& vertices, const std::vector<uint32_t>& indices);
    std::unique_ptr<CColliderShape> CConcaveMeshShape::Clone() const {
        return std::make_unique<CConcaveMeshShape>(*this);
    }

    BoundingBox ComputeTriangleAABB(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2);

    // GJK용 GetSupport 사용X
    const std::vector<Triangle>& GetCandidateTriangles(const BoundingBox& other) const;
    const std::vector<Triangle>& GetWorldTriangles() const;
    void Update(const XMMATRIX& worldMatrix) override;

    XMVECTOR GetSupport(XMVECTOR direction) const override { return XMVectorZero(); }
    // 겹치는 삼각형 인덱스만 반환(convexAABB, output index)
    void GetCandidateTrianglesBVH(const BoundingBox& queryAABB, std::vector<int>& outIndices) const;
private:
    int BuildBVHRecursive(std::vector<int>& indices, int start, int end);
    std::vector<Triangle> local;
    std::vector<Triangle> world;
    std::vector<BVHNode> nodes;      // BVH 트리 노드들
    int root_idx = -1;
    XMMATRIX inv_world_matrix;
};

struct CollisionFilter {
    uint32_t category{};    // 나의 정체성
    uint32_t mask{};        // 내가 부딪힐 대상들
};

/*
충돌 모양 데이터 제공자. 물리 계산X
* ColliderComponent 생성법
* paramaeter: shape(unique), bounds
* 사용법: ObjectFactory 참고
*/
class CColliderComponent : public CComponent
{
public:
    CColliderComponent(std::unique_ptr< CColliderShape>& otherShape, const BoundingBox& otherBox);
    ~CColliderComponent() = default;
    CColliderComponent(const CColliderComponent& other);

    void SetShape(std::unique_ptr< CColliderShape>& otherShape) { shape = std::move(otherShape); }
    CColliderShape* GetShape() const { return shape.get(); }
    void SetFillter(const CollisionFilter& f) { filter = f; }
    BoundingBox GetWorldAABB() const { return world_aabb; }

    void Update(const float deltaTime) override;
    // 디버깅용(aabb 출력)
    void Render(ID3D12GraphicsCommandList* commandList) override;

    bool Intersects(const CColliderComponent* other);
private:
    friend class CPhysicsManager;
    std::unique_ptr<CColliderShape> shape;
    BoundingBox local_aabb{};
    BoundingBox world_aabb{};   // for broad phase
    CollisionFilter filter;
    std::shared_ptr<CMesh> debug;
};
