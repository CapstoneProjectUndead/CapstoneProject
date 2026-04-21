#include "pch.h"
// Server쪽 Movement
#include "Movement.h"
#include "Object.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "Player.h"
#include "Scene.h"
#include "Room.h"


void CMovementComponent::Move(const XMFLOAT3 direction, float deltaTime)
{
    if (owner->GetCurrentSceneType() == SCENE_TYPE::CUSTOMS) return;

    // [추가] 땅에 있을 때만 조작이 가능하도록 함
    if (!owner->is_grounded) return;

    XMFLOAT3 accel{};
    if (direction.z > 0) accel = Vector3::Add(accel, owner->look);
    if (direction.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->look, -1));
    if (direction.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(owner->right, -1));
    if (direction.x > 0) accel = Vector3::Add(accel, owner->right);

    accel = Vector3::Normalize(accel);

    // 지상일 때만 속도를 즉시 결정
    owner->velocity.x = accel.x * speed;
    owner->velocity.z = accel.z * speed;
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

    OBJECT_TYPE type = owner->GetObjectType();

    switch (type)
    {
    case OBJECT_TYPE::PLAYER:
    {
        CPlayer* player = static_cast<CPlayer*>(owner);

        // 플레이어가 Custom Scene에 있으면 return
        if (player->GetCurrentSceneType() == SCENE_TYPE::CUSTOMS)
            return;
    }
    break;
    case OBJECT_TYPE::MONSTER:
        break;
    default:
        break;
    }

    // 1. 관성 없애기 및 지상 상태 속도 제어
    if (owner->is_grounded)
    {
        // [중요] 지상에 있으면 중력으로 누적된 수직 속도를 0으로 초기화해서 바닥을 뚫으려는 시도를 막음
        if (owner->velocity.y < 0) owner->velocity.y = 0.0f;
    }

    // 2. 중력 적용 (여기서 groundSeparation은 바닥에서 캐릭터를 살짝 띄워주는 보정값임)
    XMVECTOR groundSeparation = GetPhysicsManager()->ApplyGravity(owner, deltaTime);

    // 3. 이동량 계산 (바닥 보정값 적용)
    XMVECTOR finalPos = XMLoadFloat3(&owner->position) + groundSeparation + CalculatePlatform(deltaTime);

    // 4. [중요] 수평 이동량만 따로 계산 (수직 이동은 중력이 알아서 하게 둠)
    // 수평 속도와 수직 속도를 합쳐서 이동량을 구함
    XMVECTOR internalMotion = XMLoadFloat3(&owner->velocity) * deltaTime;

    // 5. 충돌 해결 (여기서 바닥이나 벽에 걸리는 걸 처리)
    ResolveCollisions(finalPos, internalMotion, deltaTime);

    ClampSpeed();

    // 6. 최종 위치 적용
    XMStoreFloat3(&owner->position, finalPos);
}

XMVECTOR CMovementComponent::CalculatePlatform(float dt)
{
    CollisionInfo info{};
    XMFLOAT3 downDelta = { 0, -0.1f, 0 };
    if (GetPhysicsManager()->Overlap(owner, downDelta, info, EColLayer::GROUND | EColLayer::OBJECT)) {
        if (info.other_object && Vector3::Length(info.other_object->velocity) > 0.001f) {
            return XMLoadFloat3(&info.other_object->velocity) * dt;
        }
    }
    return XMVectorZero();
}

void CMovementComponent::ResolveCollisions(XMVECTOR& outPos, XMVECTOR remainingMotion, float dt)
{
    // 벽/오브젝트 충돌 처리
    EColLayer wallMask = collision_mask;
    const float stepHeight = 0.2; // 오를 수 있는 최대 높이

    // Iterative Slide
    for (int i = 0; i < 3; ++i) {
        float moveLen = XMVectorGetX(XMVector3Length(remainingMotion));
        if (moveLen < 0.0001f) break; // 더 이상 갈 거리가 없으면 종료

        CollisionInfo info{};
        if (GetPhysicsManager()->Overlap(owner, Vector3::XMVectorToFloat3(remainingMotion), info, wallMask)) {
            // 1. step up
            if (TryStepUp(outPos, remainingMotion, info, stepHeight, wallMask)) {
                break;
            }
            // 2. slide
            float safeMargin = (info.depth > moveLen) ? moveLen : info.depth;   // depth 값이 비정상적일 때, 순간이동 방지
            safeMargin = max(safeMargin, 0.001f); // 최소한의 밀어내기 확보

            float normalLen = XMVectorGetX(XMVector3Length(info.normal));
            if (normalLen > 0.0001f) {
                XMVECTOR separation = -info.normal * safeMargin;
                outPos += separation;

                Slide(info.normal);

                // 남은 이동량 투영
                XMVECTOR newMotion = remainingMotion - info.normal * XMVector3Dot(remainingMotion, info.normal);

                // [추가 고려] 투영된 벡터가 원래 운동 방향과 너무 동떨어지거나 뒤로 가려고 하면 0으로 처리
                if (XMVectorGetX(XMVector3Dot(newMotion, remainingMotion)) <= 0) {
                    remainingMotion = XMVectorZero();
                }
                else {
                    remainingMotion = newMotion;
                }
            }
        }
        else {
            outPos += remainingMotion;
            break;
        }
    }
}

bool CMovementComponent::TryStepUp(XMVECTOR& outPos, XMVECTOR motion, const CollisionInfo& hit, float height, uint32_t mask)
{
    if (!owner->is_grounded || std::abs(XMVectorGetY(hit.normal)) >= 0.2f) return false;

    // 캐릭터를 stepHeight만큼 위로 올린 임시 위치 계산
    XMVECTOR stepUpOffset = XMVectorSet(0, height, 0, 0);
    XMVECTOR testPos = outPos + stepUpOffset;

    XMFLOAT3 originalPos = owner->position;
    owner->position = Vector3::XMVectorToFloat3(testPos);   // 잠시 위치 이동

    CollisionInfo stepInfo{};
    bool obstructed = GetPhysicsManager()->Overlap(owner, Vector3::XMVectorToFloat3(motion), stepInfo, mask);

    owner->position = originalPos; // 복구

    if (!obstructed) {
        // 앞 공간이 비어있다면, 이제 다시 아래로 내려서 바닥이 있는지 확인
        // 실제 엔진은 여기서 아래로 Sweep을 쏘지만, 일단은 위치 확정 후 중력이 해결하게 함
        outPos = testPos + motion;
        return true;
    }
    return false;
}

void CMovementComponent::Simulate(const XMFLOAT3& dir, float deltaTime)
{
    Move(dir, deltaTime);

    if (dir.y > 0) {
        if (owner->is_grounded) {
            owner->velocity.y = owner->jump_power;
        }
    }

    // 중력/마찰/땅 확인
    XMVECTOR groundSeparation = GetPhysicsManager()->ApplyGravity(owner, deltaTime);

    // 이동량 계산 = 기존 위치 + 바닥 보정 + 외부 발판
    XMVECTOR finalPos = XMLoadFloat3(&owner->position) + groundSeparation + CalculatePlatform(deltaTime);

    // 플레이어 이동량 계산
    XMVECTOR internalMotion = XMLoadFloat3(&owner->velocity) * deltaTime;

    // 반복 슬라이딩 + 턱 오르기 수행
    ResolveCollisions(finalPos, internalMotion, deltaTime);

    ClampSpeed();

    // 최종 위치 적용
    XMStoreFloat3(&owner->position, finalPos);
}