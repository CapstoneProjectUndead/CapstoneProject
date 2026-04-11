#include "stdafx.h"
#include "Collider.h"
#include "Object.h"
#include "Mesh.h"
#ifdef DEBUG
#include "GameFramework.h"  // 디버깅 시에 필요
#endif
#include "MeshRenderer.h"

CBoxShape::CBoxShape(XMFLOAT3 extents, XMFLOAT3 p)
{
    local.Center = p;
    local.Extents = extents;
    world = local;
#ifdef DEBUG
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, local.Extents, local.Center);
#endif
};

void CBoxShape::Render()
{
#ifdef DEBUG
    if(debug)
        debug->Render(GET_CMD_LIST);
#endif
}

void CBoxShape::Update(const XMMATRIX& worldMatrix)
{
    local.Transform(world, worldMatrix);
    XMStoreFloat4(&world.Orientation, XMQuaternionNormalize(XMLoadFloat4(&world.Orientation)));
}

// sphere
CSphereShape::CSphereShape(float r, XMFLOAT3 p)
{
    local.Radius = r;
    local.Center = p;
#ifdef DEBUG
    debug = std::make_shared<CSphereMesh>(GET_DEVICE, GET_CMD_LIST, local.Radius, local.Center);
#endif
}

CSphereShape::CSphereShape(XMFLOAT3& extents, XMFLOAT3& p)
{
    local.Center = p;

    XMVECTOR vExtents = XMLoadFloat3(&extents);
    local.Radius = XMVectorGetX(XMVector3Length(vExtents));
#ifdef DEBUG
    debug = std::make_shared<CSphereMesh>(GET_DEVICE, GET_CMD_LIST, local.Radius, local.Center);
#endif
}

void CSphereShape::Render()
{
#ifdef DEBUG
    if (debug)
        debug->Render(GET_CMD_LIST);
#endif
}

void CSphereShape::Update(const XMMATRIX& worldMatrix)
{
    local.Transform(world, worldMatrix);
}

CCapsuleShape::CCapsuleShape(float r, float h, int dir, XMFLOAT3 p)
    : radius(r), height(h), direction(dir), center(p)
{
}

void CCapsuleShape::Update(const XMMATRIX& worldMatrix)
{
    XMVECTOR vCenter = XMLoadFloat3(&center);
    world_center = XMVector3TransformCoord(vCenter, worldMatrix);

    // 축 벡터 설정 (방향에 따라)
    XMVECTOR axis = XMVectorZero();
    if (direction == 0) axis = XMVectorSet(1, 0, 0, 0); // X
    else if (direction == 1) axis = XMVectorSet(0, 1, 0, 0); // Y
    else axis = XMVectorSet(0, 0, 1, 0); // Z

    // 회전 적용
    world_axis = XMVector3Normalize(XMVector3TransformNormal(axis, worldMatrix));
    world_half_height = (height - (radius * 2.0f)) * 0.5f;
}

XMVECTOR CCapsuleShape::GetSupport(XMVECTOR direction) const
{
    // 캡슐의 중심에서 반만큼 떨어진 두 점(A, B)을 계산
    XMVECTOR A = world_center + world_axis * world_half_height;
    XMVECTOR B = world_center - world_axis * world_half_height;

    // 점 A와 B 중 방향(direction)과 내적이 더 큰 쪽을 선택
    float dotA = XMVectorGetX(XMVector3Dot(A, direction));
    float dotB = XMVectorGetX(XMVector3Dot(B, direction));

    XMVECTOR bestPoint = (dotA > dotB) ? A : B;

    // 선택된 점에 방향 벡터 방향으로 반지름만큼 더함
    return bestPoint + XMVector3Normalize(direction) * radius;
}

void CCapsuleShape::Render()
{
#ifdef DEBUG
    if (debug)
        debug->Render(GET_CMD_LIST);
#endif
}

// Convex
CConvexMeshShape::CConvexMeshShape(std::vector<XMFLOAT3> vertice)
{
    local.reserve(vertice.size());
    local = vertice;
}

void CConvexMeshShape::Update(const XMMATRIX& worldMatrix)
{
    if (world.size() != local.size()) {
        world.resize(local.size());
    }

    for (size_t i = 0; i < local.size(); ++i) {
        XMVECTOR p = XMLoadFloat3(&local[i]);
        XMVECTOR wp = XMVector3Transform(p, worldMatrix);
        XMStoreFloat3(&world[i], wp);
    }
}

// CTriangleShape
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

// Concave
CConcaveMeshShape::CConcaveMeshShape(const std::vector<XMFLOAT3>& vertices, const std::vector<uint32_t>& indices)
{
    // 로컬 삼각형 데이터 생성 (이건 변하지 않음)
    for (size_t i = 0; i < indices.size(); i += 3) {
        Triangle tri;
        tri.v[0] = vertices[indices[i]];
        tri.v[1] = vertices[indices[i + 1]];
        tri.v[2] = vertices[indices[i + 2]];
        tri.aabb = ComputeTriangleAABB(tri.v[0], tri.v[1], tri.v[2]);
        local.push_back(tri);
    }

    world.resize(local.size());

    // 로컬 데이터를 기준으로 BVH 트리 미리 빌드
    std::vector<int> triIndices(local.size());
    for (int i = 0; i < (int)triIndices.size(); ++i) triIndices[i] = i;

    nodes.reserve(local.size() * 2);
    root_idx = BuildBVHRecursive(triIndices, 0, (int)triIndices.size());
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
    inv_world_matrix = XMMatrixInverse(nullptr, worldMatrix);

    // 월드 변환
    for (size_t i = 0; i < local.size(); ++i) {
        for (int j = 0; j < 3; ++j) {
            XMVECTOR p = XMLoadFloat3(&local[i].v[j]);
            XMStoreFloat3(&world[i].v[j], XMVector3TransformCoord(p, worldMatrix));
        }
    }
}

void CConcaveMeshShape::GetCandidateTrianglesBVH(const BoundingBox& queryAABB, std::vector<int>& outIndices) const
{
    if (root_idx == -1) return;

    // 월드 공간의 쿼리(convexAABB) -> 로컬 공간 변환
    BoundingBox localQuery;
    queryAABB.Transform(localQuery, inv_world_matrix);

    std::vector<int> stack;
    stack.push_back(root_idx);

    while (!stack.empty()) {
        int currIdx = stack.back();
        stack.pop_back();

        const auto& node = nodes[currIdx];

        // 로컬 vs 로컬 검사
        if (localQuery.Intersects(node.aabb)) {
            if (node.IsLeaf()) {
                outIndices.push_back(node.triangle_idx);
            }
            else {
                stack.push_back(node.left);
                stack.push_back(node.right);
            }
        }
    }
}

int CConcaveMeshShape::BuildBVHRecursive(std::vector<int>& indices, int start, int end)
{
    int count = end - start;
    BVHNode node;

    // 1. 현재 범위의 모든 삼각형을 감싸는 전체 AABB 계산
    BoundingBox merged;
    bool first = true;
    for (int i = start; i < end; ++i) {
        if (first) { merged = local[indices[i]].aabb; first = false; }
        else BoundingBox::CreateMerged(merged, merged, local[indices[i]].aabb);
    }
    node.aabb = merged;

    // 2. 삼각형이 하나만 남으면 '잎' 노드로 생성
    if (count == 1) {
        node.triangle_idx = indices[start];
        nodes.push_back(node);
        return (int)nodes.size() - 1;
    }

    // 3. 가장 긴 축을 기준으로 정렬하여 반으로 나눔 (공간 분할)
    XMVECTOR extent = XMLoadFloat3(&node.aabb.Extents);
    int axis = 0; // X=0, Y=1, Z=2
    if (XMVectorGetY(extent) > XMVectorGetX(extent)) axis = 1;
    if (XMVectorGetZ(extent) > XMVectorGetX(extent) && XMVectorGetZ(extent) > XMVectorGetY(extent)) axis = 2;

    std::sort(indices.begin() + start, indices.begin() + end, [&](int a, int b) {
        float centerA = (&world[a].aabb.Center.x)[axis];
        float centerB = (&world[b].aabb.Center.x)[axis];
        return centerA < centerB;
        });

    int mid = start + count / 2;

    // 4. 재귀적으로 자식 노드 생성
    // (주의: push_back 시 벡터 재할당으로 인덱스가 꼬일 수 있으므로 인덱스 관리 주의)
    int nodeIdx = (int)nodes.size();
    nodes.push_back(node); // 일단 공간 확보

    int leftChild = BuildBVHRecursive(indices, start, mid);
    int rightChild = BuildBVHRecursive(indices, mid, end);

    nodes[nodeIdx].left = leftChild;
    nodes[nodeIdx].right = rightChild;

    return nodeIdx;
}

// component
CColliderComponent::CColliderComponent(std::unique_ptr<CColliderShape>& otherShape, const BoundingBox& otherBox)
    : shape{ std::move(otherShape) }, local_aabb{ otherBox }, world_aabb{otherBox}
{
#ifdef DEBUG
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, local_aabb.Extents, local_aabb.Center);
#endif
}

CColliderComponent::CColliderComponent(const CColliderComponent& other)
{
    shape = other.shape->Clone();
    world_aabb = other.GetWorldAABB();
    local_aabb = other.local_aabb;
    filter = other.filter;
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

bool CColliderComponent::Intersects(const CColliderComponent* other)
{
    // 상대방이 Box든 Sphere든 Convex든 상관없이 작동
    auto supportA = [&](XMVECTOR d) { return this->shape->GetSupport(d); };
    auto supportB = [&](XMVECTOR d) { return other->shape->GetSupport(d); };

    GJKAlgorithm::Simplex simplex;
    return GJKAlgorithm::GenericIntersects(supportA, supportB, simplex);
}