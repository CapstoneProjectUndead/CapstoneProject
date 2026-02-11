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

    // Center
    obb.Center = pos;

    // Extents (half size) → scale 반영
    obb.Extents = XMFLOAT3(
        base_extents.x * scale.x,
        base_extents.y * scale.y,
        base_extents.z * scale.z
    );

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

    sphere.Center = pos;

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

void CCapsuleShape::Update(const XMMATRIX& worldMatrix)
{
    XMFLOAT3 pos, scale;
    XMFLOAT4 rot;
    DecomposeMatrix(worldMatrix, pos, rot, scale);

    XMVECTOR center = XMLoadFloat3(&pos);
    XMVECTOR q = XMLoadFloat4(&rot);

    XMVECTOR up = XMVector3Rotate( XMVectorSet(0, 1, 0, 0), q);

    float half = (height * scale.y) * 0.5f;

    XMVECTOR topV = center + up * half;
    XMVECTOR bottomV = center - up * half;

    XMStoreFloat3(&top, topV);
    XMStoreFloat3(&bottom, bottomV);

    radius = base_radius * scale.x;
}

void CCapsuleShape::ComputeAABB(BoundingBox& outAABB) const
{
    XMVECTOR topV = XMLoadFloat3(&top);
    XMVECTOR bottomV = XMLoadFloat3(&bottom);

    XMVECTOR minV = XMVectorMin(topV, bottomV);
    XMVECTOR maxV = XMVectorMax(topV, bottomV);

    XMFLOAT3 minF, maxF;
    XMStoreFloat3(&minF, minV);
    XMStoreFloat3(&maxF, maxV);

    minF.x -= radius; minF.y -= radius; minF.z -= radius;
    maxF.x += radius; maxF.y += radius; maxF.z += radius;

    BoundingBox::CreateFromPoints(outAABB, XMLoadFloat3(&minF), XMLoadFloat3(&maxF));
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