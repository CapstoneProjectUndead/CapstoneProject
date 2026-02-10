#include "stdafx.h"
#include "Collider.h"
#include "Object.h"
#include "Mesh.h"

void CBoxColliderComponent::Update(float deltaTime)
{
    local_bounds.Transform(world_bounds, XMLoadFloat4x4(&owner->world_matrix));
}

void CBoxColliderComponent::SetLocalBounds(const BoundingBox& bounds)
{
    local_bounds = bounds;
}