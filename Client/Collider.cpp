#include "stdafx.h"
#include "Collider.h"
#include "Object.h"
#include "Mesh.h"
#include "GameFramework.h"  // 디버깅 시에 필요
#include "MeshRenderer.h"

CBoxShape::CBoxShape(XMFLOAT3 extents, XMFLOAT3& p)
{
    local.Center = p;
    local.Extents = extents;
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, local.Extents, local.Center);
};

void CBoxShape::Render()
{
#ifdef DEBUG
    debug->Render(GET_CMD_LIST);
#endif
}

void CBoxShape::Update(const XMMATRIX& worldMatrix)
{
    local.Transform(world, worldMatrix);
    XMStoreFloat4(&world.Orientation, XMQuaternionNormalize(XMLoadFloat4(&world.Orientation)));
}

// sphere
CSphereShape::CSphereShape(float r, XMFLOAT3& p)
{
    local.Radius = r;
    local.Center = p;
    debug = std::make_shared<CSphereMesh>(GET_DEVICE, GET_CMD_LIST, local.Radius, local.Center);
}

CSphereShape::CSphereShape(XMFLOAT3& extents, XMFLOAT3& p)
{
    local.Center = p;

    XMVECTOR vExtents = XMLoadFloat3(&extents);
    local.Radius = XMVectorGetX(XMVector3Length(vExtents));
    debug = std::make_shared<CSphereMesh>(GET_DEVICE, GET_CMD_LIST, local.Radius, local.Center);
}

void CSphereShape::Render()
{
#ifdef DEBUG
    debug->Render(GET_CMD_LIST);
#endif
}

void CSphereShape::Update(const XMMATRIX& worldMatrix)
{
    local.Transform(world, worldMatrix);
}

CConvexMeshShape::CConvexMeshShape(std::vector<XMFLOAT3>& vertice)
{
    local.reserve(vertice.size());
    local = vertice;
}

void CConvexMeshShape::Update(const XMMATRIX& worldMatrix)
{
    if (world.size() != local.size()) {
        world.resize(local.size());
    }

    for (size_t i = 0; i < local.size(); ++i)
    {
        XMVECTOR p = XMLoadFloat3(&local[i]);
        XMVECTOR wp = XMVector3Transform(p, worldMatrix);
        XMStoreFloat3(&world[i], wp);
    }
}

// Concave
XMVECTOR CTriangleShape::GetSupport(XMVECTOR direction) const
{
    float maxDot = -FLT_MAX;
    XMVECTOR bestVertex = v[0];

    for (int i = 0; i < 3; ++i) {
        float dot = XMVectorGetX(XMVector3Dot(v[i], direction));
        if (dot > maxDot) {
            maxDot = dot;
            bestVertex = v[i];
        }
    }
    return bestVertex;
}

BoundingBox CConcaveMeshShape::ComputeTriangleAABB(const XMFLOAT3& v0, const XMFLOAT3& v1, const XMFLOAT3& v2)
{
    // 1. 각 축의 최솟값과 최댓값 초기화
    XMVECTOR p0 = XMLoadFloat3(&v0);
    XMVECTOR p1 = XMLoadFloat3(&v1);
    XMVECTOR p2 = XMLoadFloat3(&v2);

    XMVECTOR minVec = XMVectorMin(p0, XMVectorMin(p1, p2));
    XMVECTOR maxVec = XMVectorMax(p0, XMVectorMax(p1, p2));

    // 2. Center(중심)와 Extents(반지름 크기) 계산
    // Center = (Max + Min) / 2
    // Extents = (Max - Min) / 2
    XMVECTOR centerVec = (maxVec + minVec) * 0.5f;
    XMVECTOR extentsVec = (maxVec - minVec) * 0.5f;

    BoundingBox aabb;
    XMStoreFloat3(&aabb.Center, centerVec);
    XMStoreFloat3(&aabb.Extents, extentsVec);

    return aabb;
}

CConcaveMeshShape::CConcaveMeshShape(const std::vector<XMFLOAT3>& vertices, const std::vector<uint32_t>& indices)
{
    // 1. 모든 삼각형 데이터를 미리 생성
    for (size_t i = 0; i < indices.size(); i += 3) {
        Triangle tri;
        tri.v[0] = vertices[indices[i]];
        tri.v[1] = vertices[indices[i + 1]];
        tri.v[2] = vertices[indices[i + 2]];

        // 각 삼각형의 개별 AABB를 미리 계산 (최적화용)
        tri.aabb = ComputeTriangleAABB(tri.v[0], tri.v[1], tri.v[2]);
        local.push_back(tri);
    }
}

const std::vector<CConcaveMeshShape::Triangle>& CConcaveMeshShape::GetCandidateTriangles(const BoundingBox& other) const
{
    static std::vector<Triangle> candidates;
    candidates.clear();
    for (const auto& tri : world) {
        if (other.Intersects(tri.aabb)) {
            candidates.push_back(tri);
        }
    }
    return candidates;
}

const std::vector<CConcaveMeshShape::Triangle>& CConcaveMeshShape::GetWorldTriangles() const
{
    return world;
}

void CConcaveMeshShape::Update(const XMMATRIX& worldMatrix)
{
    if (world.size() != local.size()) {
        world.resize(local.size());
    }

    for (size_t i = 0; i < local.size(); ++i) {
        const auto& localTri = local[i];
        auto& worldTri = world[i];

        // 세 정점을 월드 좌표로 변환
        for (int j = 0; j < 3; ++j) {
            XMVECTOR localPos = XMLoadFloat3(&localTri.v[j]);
            // w성분에 1.0을 넣어 좌표 변환 (Translation 포함) 적용
            XMVECTOR worldPos = XMVector3TransformCoord(localPos, worldMatrix);
            XMStoreFloat3(&worldTri.v[j], worldPos);
        }

        // 월드 AABB 업데이트
        worldTri.aabb = ComputeTriangleAABB(worldTri.v[0], worldTri.v[1], worldTri.v[2]);
    }
}

// component
CColliderComponent::CColliderComponent(std::unique_ptr<CColliderShape>& otherShape, const BoundingBox& otherBox)
    : shape{ std::move(otherShape) }, local_aabb{ otherBox }
{
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, local_aabb.Extents, local_aabb.Center);
}

void CColliderComponent::Update(const float deltaTime)
{
    XMMATRIX worldMatrix{ XMLoadFloat4x4(&owner->world_matrix) };

    local_aabb.Transform(world_aabb, worldMatrix);
    shape->Update(worldMatrix);
}

void CColliderComponent::Render(ID3D12GraphicsCommandList* commandList)
{
#ifdef DEBUG
    if (debug) debug->Render(commandList);
    if (shape) shape->Render();
#endif
}

bool CColliderComponent::Intersects(const CColliderComponent* other) {
    // 상대방이 Box든 Sphere든 Convex든 상관없이 작동
    auto supportA = [&](XMVECTOR d) { return this->shape->GetSupport(d); };
    auto supportB = [&](XMVECTOR d) { return other->shape->GetSupport(d); };

    GJKAlgorithm::Simplex simplex;
    return GJKAlgorithm::GenericIntersects(supportA, supportB, simplex);
}