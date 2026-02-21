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

void CMovementComponent::Jump()
{
    if (owner->is_grounded) {
        owner->velocity.y = owner->jump_power; // 예: 10.0f 처럼 즉시 대입
    }
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

    // 중력/마찰/땅 확인
    CPhysicsManager::GetInstance().ApplyGravity(owner, deltaTime);

    // 이동량 계산
    XMFLOAT3 delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    float moveDist = Vector3::Length(delta);
    if (moveDist < 0.0001f) return; // 움직임이 없으면 스킵

    GJKAlgorithm::CollisionInfo info{};
    // 중복 코드 람다로 처리
    auto ResolveCollision = [&]() {
        // overlap된 만큼 밀어내기
        XMVECTOR separation = info.normal * info.depth;
        XMVECTOR curPos = XMLoadFloat3(&owner->position);
        XMStoreFloat3(&owner->position, curPos + separation);

        Slide(info.normal);
        };

    // 지형 충돌(벽)
    XMFLOAT3 moveDir = Vector3::Normalize(delta);
    if (CPhysicsManager::GetInstance().Raycast(owner->position, moveDir, moveDist, info)) {
        ResolveCollision();
    }

    // 오브젝트 충돌(Table 등)
    delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    if (CPhysicsManager::GetInstance().Overlap(owner, delta, info)) {
        if (XMVectorGetY(info.normal) < 0) info.normal = -info.normal;
        ResolveCollision();
    }

    // 최종 이동
    delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    owner->position = Vector3::Add(owner->position, delta);
}

void CMovementComponent::Simulate(const XMFLOAT3& dir, float dt)
{
    // ----------------------------
    // 1. 입력에 따른 가속 & 속도 계산
    // ----------------------------
    XMFLOAT3 accel{};
    if (dir.z > 0) accel = Vector3::Add(accel, owner->look);
    if (dir.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->look, -1));
    if (dir.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->right, -1));
    if (dir.x > 0) accel = Vector3::Add(accel, owner->right);

    // 속도 갱신
    owner->velocity = Vector3::Add(owner->velocity, Vector3::ScalarProduct(accel, speed * dt));
    ClampSpeed(); // 최대 속도 제한

    // ------------------------------------------
    // 2. 중력 적용 (PhysicsManager에서 가져온 로직)
    // ------------------------------------------

    CPhysicsManager::GetInstance().ApplyGravity(owner, dt);

    // ------------------------------------------
    // 3. 이동 시뮬레이션 (Raycast + Overlap)
    // ------------------------------------------
    // 예상 이동량
    XMFLOAT3 delta = Vector3::ScalarProduct(owner->velocity, dt);
    float moveDist = Vector3::Length(delta);

    // 움직임이 거의 없으면 종료 (불필요한 연산 방지)
    if (moveDist < 0.0001f) 
        return;

    auto collider = owner->GetComponent<CColliderComponent>();

    if (collider) {
        GJKAlgorithm::CollisionInfo info{};

        // 지형 충돌 (Raycast) - 벽/경사로 체크
        XMFLOAT3 moveDir = Vector3::Normalize(delta);
        if (CPhysicsManager::GetInstance().Raycast(owner->position, moveDir, moveDist, info)) {

            // 충돌 깊이만큼 밀어내기 (Separation)
            XMVECTOR n = info.normal;
            float d = info.depth;
            XMVECTOR separation = n * d;

            // 위치를 즉시 보정해야 다음 Overlap 계산이 정확해짐
            XMVECTOR curPos = XMLoadFloat3(&owner->position);
            XMStoreFloat3(&owner->position, curPos + separation);

            // 미끄러짐 처리 (속도 변경)
            Slide(info.normal);

            // 변경된 속도로 delta 재계산 (남은 시간만큼만 이동하거나 해야 하지만, 보통 전체 dt로 다시 구함)
            delta = Vector3::ScalarProduct(owner->velocity, dt);
        }

        // 오브젝트 충돌 (Overlap) - 테이블, 다른 플레이어 등
        if (CPhysicsManager::GetInstance().Overlap(owner, delta, info)) {
            XMVECTOR n = info.normal;
            float d = info.depth;

            // 바닥 아래로 밀리는 현상 방지 (Update에 있던 로직)
            if (XMVectorGetY(n) < 0) {
                n = -n;
            }

            // 밀어내기
            XMVECTOR separation = n * d;
            XMVECTOR curPos = XMLoadFloat3(&owner->position);
            XMStoreFloat3(&owner->position, curPos + separation);

            // 미끄러짐
            Slide(n); // Slide 함수 인자가 XMFLOAT3면 변환 필요

            // delta 재계산
            delta = Vector3::ScalarProduct(owner->velocity, dt);
        }
    }

    // ----------------
    // 4. 최종 위치 적용 
    // ----------------
    owner->position = Vector3::Add(owner->position, delta);
}