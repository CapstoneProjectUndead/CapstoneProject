#pragma once
#include "Component.h"

class CObject;
struct CollisionInfo;

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
	// 플레이어가 위에 있는 움직이는 물체의 속도 return
	XMVECTOR CalculatePlatform(float dt);
	// 충돌 처리
	void ResolveCollisions(XMVECTOR& outPos, XMVECTOR remainingMotion, float dt);
	// 턱 오르기
	bool TryStepUp(XMVECTOR& outPos, XMVECTOR motion, const CollisionInfo& hit, float height, uint32_t mask);

	void Move(const XMFLOAT3 direction, float deltaTime);
	// 벽(Normal)을 따라 Slide
	void Slide(const XMFLOAT3& normal);
	void Slide(const XMVECTOR& normal);
	void Jump();

	// 최대 속도 제한
	void ClampSpeed();
	// Y 위치 제한(무한 낙하 방지)
	void ClampY();
	void SetSpeed(const float otherSpeed) { speed = otherSpeed; }
	float GetSpeed() const { return speed; }

	// 서버에서 받은 결과를 바탕으로 재시뮬
	void Simulate(const XMFLOAT3& dir, float deltaTime);

	bool is_fly{ false }; // 무적 모드(디버깅용)
private:
	float speed{ 10.0f };
	float max_speed{ 30.0f };
};