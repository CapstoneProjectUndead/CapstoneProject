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

    // 중력 및 지면 체크
    CollisionInfo groundInfo{};
    XMFLOAT3 downDelta = { 0, -0.1f, 0 };
    // [중요] Overlap 시 충돌한 상대방(other) 정보를 받아올 수 있도록 수정되었다고 가정
    if (CPhysicsManager::GetInstance().Overlap(owner, downDelta, EColLayer::GROUND | EColLayer::OBJECT, groundInfo)) {

        // --- 물체 위에 서 있기 로직 ---
        // 만약 딛고 있는 물체가 움직이는 물체(속도가 있음)라면
        if (groundInfo.other_object) { // CollisionInfo에 CObject* 추가 필요
            XMFLOAT3 otherVel = groundInfo.other_object->velocity;
            XMVECTOR platformMovement = XMLoadFloat3(&otherVel) * deltaTime;

            // 발판의 이동량을 플레이어 위치에 강제 적용
            XMVECTOR curPos = XMLoadFloat3(&owner->position);
            XMStoreFloat3(&owner->position, curPos + platformMovement);
        }
    }

    // 반복 슬라이딩 이동 (Iterative Slide)
    XMVECTOR currentPosition = XMLoadFloat3(&owner->position);
    XMVECTOR velocity = XMLoadFloat3(&owner->velocity);
    XMVECTOR remainingMotion = velocity * deltaTime; // 이번 프레임에 가야 할 총 거리

    // 벽/오브젝트 충돌 처리
    uint32_t wallMask = EColLayer::WALL | EColLayer::OBJECT;
    const float stepHeight = 0.2; // 오를 수 있는 최대 높이

    // Iterative Slide
    const float slop{ 0.001f };
    for (int i = 0; i < 3; ++i) {
        float moveLen = XMVectorGetX(XMVector3Length(remainingMotion));
        if (moveLen < 0.0001f) break; // 더 이상 갈 거리가 없으면 종료

        CollisionInfo info{};
        if (CPhysicsManager::GetInstance().Overlap(owner, Vector3::XMVectorToFloat3(remainingMotion), wallMask, info)) {
            bool stepSucceeded = false;
            if (owner->is_grounded && std::abs(XMVectorGetY(info.normal)) < 0.2f) {
                // 캐릭터를 stepHeight만큼 위로 올린 임시 위치 계산
                XMVECTOR stepUpOffset = XMVectorSet(0, stepHeight, 0, 0);
                XMVECTOR testPos = currentPosition + stepUpOffset;

                // 위로 올린 위치에서 원래 가려던 방향(remainingMotion)으로 Overlap 체크
                // (이때는 살짝 띄운 상태이므로 벽 상단을 통과할 수 있음)
                owner->position = Vector3::XMVectorToFloat3(testPos); // 잠시 위치 이동
                CollisionInfo stepInfo{};
                if (!CPhysicsManager::GetInstance().Overlap(owner, Vector3::XMVectorToFloat3(remainingMotion), wallMask, stepInfo))
                {
                    // 앞 공간이 비어있다면, 이제 다시 아래로 내려서 바닥이 있는지 확인
                    // 실제 엔진은 여기서 아래로 Sweep을 쏘지만, 일단은 위치 확정 후 중력이 해결하게 함
                    currentPosition = testPos + remainingMotion;
                    stepSucceeded = true;
                }
                // 위치 복구 (검사 끝)
                owner->position = Vector3::XMVectorToFloat3(currentPosition);
            }

            // 턱 오르기 성공 시, 이번 루프의 이동은 끝난 것으로 간주하거나 남은 거리 조절
            if (stepSucceeded) break;
            else {
                // 기존 슬라이딩 로직 수행
                XMVECTOR separation = -info.normal * (info.depth + slop);
                currentPosition += separation;

                Slide(info.normal);

                XMVECTOR slideNormal = info.normal;
                XMVECTOR dot = XMVector3Dot(remainingMotion, slideNormal);
                remainingMotion = remainingMotion - (slideNormal * dot);
            }
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