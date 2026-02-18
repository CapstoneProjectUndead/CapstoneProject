#include "stdafx.h"
#include "Movement.h"
#include "Object.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "Player.h"

void CMovementComponent::Move(const XMFLOAT3 direction, float deltaTime)
{
	XMFLOAT3 accel{};

	if (direction.z > 0) accel = Vector3::Add(accel, owner->look);
	if (direction.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->look, -1));
	if (direction.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->right, -1));
	if (direction.x > 0) accel = Vector3::Add(accel, owner->right);

    accel = Vector3::Normalize(accel);

	// 가속도 적용: velocity += accel * speed * deltaTime
	owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(accel, speed * deltaTime));
}

void CMovementComponent::ClampSpeed()
{
	float lenXZ = sqrtf(owner->velocity.x * owner->velocity.x + owner->velocity.z * owner->velocity.z);
	if (lenXZ > max_speed) {
		float ratio = max_speed / lenXZ;
		owner->velocity.x *= ratio;
		owner->velocity.z *= ratio;
	}
}

void CMovementComponent::Slide(const XMFLOAT3& normal)
{
    XMVECTOR v = XMLoadFloat3(&owner->velocity);
    XMVECTOR n = XMLoadFloat3(&normal);

    XMVECTOR dot = XMVector3Dot(v, n);

    XMVECTOR result = v - n * dot;

    XMStoreFloat3(&owner->velocity, result);
}

void CMovementComponent::Slide(const XMVECTOR& normal)
{
    XMVECTOR v = XMLoadFloat3(&owner->velocity);

    XMVECTOR dot = XMVector3Dot(v, normal);

    XMVECTOR result = v - normal * dot;

    XMStoreFloat3(&owner->velocity, result);
}

void CMovementComponent::Update(const float deltaTime)
{
	if (owner == nullptr)
		return;

	// 상대 플레이어이라면 return
	auto p = dynamic_cast<CPlayer*>(owner);
	if (p != nullptr && !p->GetIsMyPlayer())
		return;

	ClampSpeed();
    CPhysicsManager::GetInstance().ApplyGravity(owner, deltaTime);

    XMFLOAT3 delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    float moveDist = Vector3::Length(delta);
    if (moveDist < 0.0001f) return; // 움직임이 없으면 스킵

    GJKAlgorithm::CollisionInfo info{};

    // 지형 충돌(벽)
    XMFLOAT3 moveDir = Vector3::Normalize(delta);
    if (CPhysicsManager::GetInstance().Raycast(owner->position, moveDir, moveDist, info)) {
        XMVECTOR n = info.normal;
        float d = info.depth;
        // Overlap된 깊이만큼 반대로 이동
        XMVECTOR separation = n * (d);
        XMVECTOR curPos = XMLoadFloat3(&owner->position);
        XMStoreFloat3(&owner->position, curPos + separation);

        Slide(info.normal);
    }

    // 오브젝트 충돌(Table 등)
    delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    if (CPhysicsManager::GetInstance().Overlap(owner, delta, info)) {
        XMVECTOR n = info.normal;
        float d = info.depth;

        // 바닥 아래로 밀리는 현상 방지
        if (XMVectorGetY(n) < 0) {
            n = -n;
        }
        // Overlap된 깊이만큼 반대로 이동
        XMVECTOR separation = n * (d);
        XMVECTOR curPos = XMLoadFloat3(&owner->position);
        XMStoreFloat3(&owner->position, curPos + separation);

        Slide(n);

        // 보정된 속도로 delta 재계산
        delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    }
    owner->position = Vector3::Add(owner->position, delta);
}

void CMovementComponent::Simulate(const XMFLOAT3& dir, float dt)
{
    // 1. 입력 → 가속도
    XMFLOAT3 accel{};
    if (dir.z > 0) accel = Vector3::Add(accel, owner->look);
    if (dir.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->look, -1));
    if (dir.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->right, -1));
    if (dir.x > 0) accel = Vector3::Add(accel, owner->right);

    // 2. 가속 적용 (Velocity 갱신)
    owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(accel, speed * dt));

    // 3. 최대 속도 제한
    float lenXZ = sqrtf(owner->velocity.x * owner->velocity.x + owner->velocity.z * owner->velocity.z);
    if (lenXZ > max_speed) {
        float ratio = max_speed / lenXZ;
        owner->velocity.x *= ratio;
        owner->velocity.z *= ratio;
    }

    // 4. 마찰 (Friction) - 속도를 줄이는 건 여기서 해도 됨
    float speedLen = Vector3::Length(owner->velocity);
    float decel = owner->friction * dt; // owner->friction 확인 필요
    if (decel > speedLen) decel = speedLen;

    owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(owner->velocity, -decel, true));
}