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
	// 벽(Normal)을 따라 Slide
	void Slide(const XMFLOAT3& normal);
	void Slide(const XMVECTOR& normal);
	void Jump();

	// 최대 속도 제한
	void ClampSpeed();
	void SetSpeed(const float otherSpeed) { speed = otherSpeed; }
	float GetSpeed() const { return speed; }

	// 서버에서 받은 결과를 바탕으로 재시뮬
	void Simulate(const XMFLOAT3& dir, float deltaTime);
private:
	float speed{ 10.0f };
	float max_speed{ 30.0f };
};