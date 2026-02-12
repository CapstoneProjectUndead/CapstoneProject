#include "stdafx.h"
#include "Collider.h"
#include "Object.h"

inline void DecomposeMatrix(const XMMATRIX& world, XMFLOAT3& outPos, XMFLOAT4& outRot, XMFLOAT3& outScale)
{
    XMVECTOR scale;
    XMVECTOR rot;
    XMVECTOR trans;

    XMMatrixDecompose(&scale, &rot, &trans, world);

    XMStoreFloat3(&outScale, scale);
    XMStoreFloat4(&outRot, rot);
    XMStoreFloat3(&outPos, trans);
}

void CBoxShape::Update(const XMMATRIX& worldMatrix)
{
    XMFLOAT3 pos, scale;
    XMFLOAT4 rot;
    DecomposeMatrix(worldMatrix, pos, rot, scale);

    // pivot을 world space로 변환
    XMVECTOR p = XMLoadFloat3(&pivot);
    XMVECTOR q = XMLoadFloat4(&rot);
    XMVECTOR pivotWorld = XMVector3Rotate(p, q);

    XMFLOAT3 pivotOffset;
    XMStoreFloat3(&pivotOffset, pivotWorld);

    // Center = world position + pivot offset
    obb.Center = Vector3::Add(pos, pivotOffset);

    // Extents (half size) → scale 반영
    obb.Extents = Vector3::Multiply(base_extents, scale);

    // Orientation
    obb.Orientation = rot;
}

void CBoxShape::ComputeAABB(BoundingBox& outAABB) const
{
    XMFLOAT3 corners[8];
    obb.GetCorners(corners);

    XMVECTOR minV = XMLoadFloat3(&corners[0]);
    XMVECTOR maxV = minV;

    for (int i = 1; i < 8; i++) {
        XMVECTOR v = XMLoadFloat3(&corners[i]);
        minV = XMVectorMin(minV, v);
        maxV = XMVectorMax(maxV, v);
    }

    BoundingBox::CreateFromPoints(outAABB, minV, maxV);
}

void CSphereShape::Update(const XMMATRIX& worldMatrix)
{
    XMFLOAT3 pos, scale;
    XMFLOAT4 rot;
    DecomposeMatrix(worldMatrix, pos, rot, scale);

    // pivot을 world space로 변환
    XMVECTOR p = XMLoadFloat3(&pivot);
    XMVECTOR q = XMLoadFloat4(&rot);
    XMVECTOR pivotWorld = XMVector3Rotate(p, q);

    XMFLOAT3 pivotOffset;
    XMStoreFloat3(&pivotOffset, pivotWorld);

    // Center = world position + pivot offset
    sphere.Center = Vector3::Add(pos, pivotOffset);

    float uniformScale = scale.x; // uniform 가정
    sphere.Radius = radius * uniformScale;
}

void CSphereShape::ComputeAABB(BoundingBox& outAABB) const
{
    XMFLOAT3 c = sphere.Center;
    float r = sphere.Radius;

    XMVECTOR minV = XMVectorSet(c.x - r, c.y - r, c.z - r, 0);
    XMVECTOR maxV = XMVectorSet(c.x + r, c.y + r, c.z + r, 0);

    BoundingBox::CreateFromPoints(outAABB, minV, maxV);
}

// component
void CColliderComponent::Update(const float deltaTime)
{
    shape->Update(XMLoadFloat4x4(&owner->world_matrix));
    shape->ComputeAABB(aabb);
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