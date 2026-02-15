#include "stdafx.h"
#include "Collider.h"
#include "Object.h"
#include "Mesh.h"
#include "GameFramework.h"  // 디버깅 시에 필요
#include "MeshRenderer.h"

void CBoxShape::Render()
{
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, GET_CMD_LIST, local.Extents, local.Center);

    debug->Render(GET_CMD_LIST);
}

void CBoxShape::Update(const XMMATRIX& worldMatrix)
{
    local.Transform(world, worldMatrix);
    XMStoreFloat4(&world.Orientation, XMQuaternionNormalize(XMLoadFloat4(&world.Orientation)));
}

void CSphereShape::Render()
{
    debug = std::make_shared<CSphereMesh>(GET_DEVICE, GET_CMD_LIST, local.Radius, local.Center);

    debug->Render(GET_CMD_LIST);
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
    world.resize(local.size());

    for (size_t i = 0; i < local.size(); i++)
    {
        XMVECTOR p = XMLoadFloat3(&local[i]);
        XMVECTOR wp = XMVector3Transform(p, worldMatrix);
        XMStoreFloat3(&world[i], wp);
    }
}

bool CConvexMeshShape::Intersects(const CConvexMeshShape& other) const
{
    return false;
}

CColliderComponent::CColliderComponent(std::unique_ptr<CColliderShape>& otherShape, const BoundingBox& otherBox)
    : shape{ std::move(otherShape) }, local_aabb{ otherBox }
{
}

// component
void CColliderComponent::Update(const float deltaTime)
{
    XMMATRIX worldMatrix{ XMLoadFloat4x4(&owner->world_matrix) };

    local_aabb.Transform(world_aabb, worldMatrix);
    shape->Update(worldMatrix);
}

void CColliderComponent::Render(ID3D12GraphicsCommandList* commandList)
{
#ifdef DEBUG
    debug = std::make_shared<CCubeMesh>(GET_DEVICE, commandList, local_aabb.Extents, local_aabb.Center);

    debug->Render(commandList);
    if (shape) shape->Render();
#endif // DEBUG
}

bool CColliderComponent::Intersects(const CColliderComponent* other)
{
    // OBB
    if (auto obb = shape->GetOBB()) {
        if (auto otherShape = other->shape->GetOBB())
            return obb->Intersects(*otherShape);

        if (auto otherShape = other->shape->GetSphere())
            return obb->Intersects(*otherShape);
    }

    // Sphere
    if (auto obb = shape->GetSphere()) {
        if (auto otherShape = other->shape->GetOBB())
            return obb->Intersects(*otherShape);

        if (auto otherShape = other->shape->GetSphere())
            return obb->Intersects(*otherShape);
    }

    return false;
}
