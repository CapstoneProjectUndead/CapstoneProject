#pragma once
#include "Component.h"

class CObject;

/*
* 사용 예시
if (auto move = GetComponent<CMovementComponent>())
	move->Move(dir, dt);

GetComponent<>() -> CObject에 정의됨
*/
class CMovementComponent : public CComponent
{
public:
	CMovementComponent() = default;
	void Update(const float deltaTime) override;
	void Move(const XMFLOAT3 direction, float deltaTime);

	void SetSpeed(const float otherSpeed) { speed = otherSpeed; }
	float GetSpeed() const { return speed; }
private:
	float speed{ 10.0f };
	float max_speed{ 30.0f };
	float friction{ 8.0f };
};