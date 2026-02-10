#include "stdafx.h"
#include "PhysicsManager.h"
#include "Collider.h"
#include "Object.h"
#include "Movement.h"

void CPhysicsManager::Update(float dt)
{
    // 1) 모든 collider world_bounds 갱신
    for (auto& col : colliders) {
        if (!col->owner) continue;

        auto moveCom = col->owner->GetComponent<CMovementComponent>();
        if (!moveCom) continue;

        Move(col.get(), moveCom, moveCom->desired_move);
    }

    // 3) SAP broad-phase
    std::vector<std::pair<CColliderComponent*, CColliderComponent*>> pairs;
    BroadPhaseSAP(pairs);

    // 4) Narrow-phase 충돌 처리
    for (auto& p : pairs)
    {
        if (p.first->Intersect(p.second))
        {
        }
    }
}

void CPhysicsManager::BroadPhaseSAP(std::vector<std::pair<CColliderComponent*, CColliderComponent*>>& outPairs)
{
    std::vector<EndPoint> endpoints;
    endpoints.reserve(colliders.size() * 2);

    for (auto col : colliders)
    {
        const auto& box = col->world_bounds;
        endpoints.push_back(EndPoint{ box.Center.x - box.Extents.x, col.get(), true});
        endpoints.push_back(EndPoint{ box.Center.x + box.Extents.x, col.get(), false });
    }

    std::sort(endpoints.begin(), endpoints.end(),
        [](const EndPoint& a, const EndPoint& b) {
            return a.value < b.value;
        });

    std::vector<CColliderComponent*> active;

    for (auto& ep : endpoints)
    {
        if (ep.is_min)
        {
            for (auto other : active)
                outPairs.push_back({ ep.col, other });

            active.push_back(ep.col);
        }
        else
        {
            active.erase(std::remove(active.begin(), active.end(), ep.col), active.end());
        }
    }
}

void CPhysicsManager::Move(CColliderComponent* collider, CMovementComponent* moveCom, const XMFLOAT3& delta)
{
    CObject* obj = collider->owner;
    if (!obj || !moveCom) return;

    XMFLOAT3 oldPos = obj->position;

    obj->position = Vector3::Add(obj->position, delta);
    collider->Update(0);

    for (auto& c : colliders)
    {
        if (c.get() == collider) continue;
        if (c->owner == obj)     continue;

        if (collider->Intersect(c.get()))
        {
            obj->position = oldPos;
            collider->Update(0);

            XMFLOAT3 normal = ComputeCollisionNormal(collider, c.get());
            moveCom->Slide(normal);
            return;
        }
    }
}

XMFLOAT3 CPhysicsManager::ComputeCollisionNormal(CColliderComponent* a, CColliderComponent* b)
{
    const auto& A = a->world_bounds;
    const auto& B = b->world_bounds;

    XMFLOAT3 n{ 0,0,0 };

    float dx = (A.Center.x - B.Center.x);
    float px = (A.Extents.x + B.Extents.x) - fabsf(dx);

    float dy = (A.Center.y - B.Center.y);
    float py = (A.Extents.y + B.Extents.y) - fabsf(dy);

    float dz = (A.Center.z - B.Center.z);
    float pz = (A.Extents.z + B.Extents.z) - fabsf(dz);

    // 가장 적게 겹친 축을 충돌 법선으로 사용
    if (px < py && px < pz)
        n = XMFLOAT3{ dx > 0 ? 1.0f : -1.0f, 0, 0 };
    else if (py < pz)
        n = XMFLOAT3{ 0, dy > 0 ? 1.0f : -1.0f, 0 };
    else
        n = XMFLOAT3{ 0, 0, dz > 0 ? 1.0f : -1.0f };

    return n;
}
