#include "stdafx.h"
#include "Movement.h"
#include "Object.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "Player.h"
#include "MyPlayer.h"

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
    auto player = dynamic_cast<CPlayer*>(owner);
    if (player != nullptr && !player->GetIsMyPlayer())
        return;
    // 싱글 플레이일 경우에만, CMovementComponent::Update를 실행한다. 
    auto myPlayer = static_cast<CMyPlayer*>(player);
    if (!myPlayer->GetIsSingle())
        return;

    // Update
    ClampSpeed();

    // 중력/마찰/땅 확인
    CPhysicsManager::GetInstance().ApplyGravity(owner, deltaTime);

    // 반복 슬라이딩 이동 (Iterative Slide)
    XMVECTOR currentPosition = XMLoadFloat3(&owner->position);
    XMVECTOR velocity = XMLoadFloat3(&owner->velocity);
    XMVECTOR remainingMotion = velocity * deltaTime; // 이번 프레임에 가야 할 총 거리

    // 벽/오브젝트 충돌 처리
    uint32_t wallMask = EColLayer::WALL | EColLayer::OBJECT;
    for (int i = 0; i < 3; ++i) {
        float moveLen = XMVectorGetX(XMVector3Length(remainingMotion));
        if (moveLen < 0.0001f) break; // 더 이상 갈 거리가 없으면 종료

        CollisionInfo info{};
        XMFLOAT3 fRemaining;
        XMStoreFloat3(&fRemaining, remainingMotion);
        // 이동하려는 예상치(remainingMotion)만큼 미리 체크
        if (CPhysicsManager::GetInstance().Overlap(owner, fRemaining, wallMask, info)) {
            // 밀어내기
            XMVECTOR separation = -info.normal * (info.depth + 0.001f);
            currentPosition += separation;

            // 속도 슬라이딩 (남은 이동 방향을 벽을 따라 깎음)
            Slide(info.normal);

            // 남은 이동량(remainingMotion) 재계산
            // 이미 부딪혔으므로, 부딪힌 면 방향의 에너지를 제거하고 남은 벡터를 구함
            XMVECTOR slideNormal = info.normal;
            XMVECTOR dot = XMVector3Dot(remainingMotion, slideNormal);
            remainingMotion = remainingMotion - (slideNormal * dot);
        }
        else {
            currentPosition += remainingMotion;
            break;
        }
    }
    
    // 최종 위치 적용
    XMStoreFloat3(&owner->position, currentPosition);
}

void CMovementComponent::Simulate(const XMFLOAT3& dir, float deltaTime)
{
    Move(dir, deltaTime);

    if (dir.y > 0) {
        if (owner->is_grounded) {
            owner->velocity.y = owner->jump_power;
        }
    }

    // 최대 속도 제한
    ClampSpeed();

    // 중력/마찰/땅 확인
    CPhysicsManager::GetInstance().ApplyGravity(owner, deltaTime);

    // 이동량 계산
    XMFLOAT3 delta = Vector3::ScalarProduct(owner->velocity, deltaTime);
    float moveDist = Vector3::Length(delta);
    if (moveDist < 0.0001f) return; // 움직임이 없으면 스킵

    CollisionInfo info{};
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