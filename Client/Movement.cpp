#include "stdafx.h"
#include "Movement.h"
#include "Object.h"

void CMovementComponent::Move(const XMFLOAT3 direction, float deltaTime)
{
	XMFLOAT3 accel{};

	if (direction.z > 0) accel = Vector3::Add(accel, owner->look);
	if (direction.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->look, -1));
	if (direction.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->right, -1));
	if (direction.x > 0) accel = Vector3::Add(accel, owner->right);

	// 가속도 적용: velocity += accel * speed * deltaTime
	owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(accel, speed * deltaTime));
}

void CMovementComponent::Update(const float deltaTime)
{
	// 중력 적용
	//velocity = Vector3::Add(velocity, Vector3::ScalarProduct(gravity, deltaTime));

	// 최대 속도 제한
	float lenXZ = sqrtf(owner->velocity.x * owner->velocity.x + owner->velocity.z * owner->velocity.z);
	if (lenXZ > max_speed) {
		float ratio = max_speed / lenXZ;
		owner->velocity.x *= ratio;
		owner->velocity.z *= ratio;
	}

	// 이동
	owner->position = Vector3::Add(owner->position, Vector3::ScalarProduct(owner->velocity, deltaTime));

	// 감속(마찰)
	float speedLen = Vector3::Length(owner->velocity);
	float decel = friction * deltaTime;
	if (decel > speedLen) decel = speedLen;

	owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(owner->velocity, -decel, true));
}