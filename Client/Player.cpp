#include "stdafx.h"
#include "Player.h"

#undef min
#undef max

// Player
CPlayer::CPlayer()
	: CCharacter()
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CPlayer::Update(float elapsedTime)
{    
     CCharacter::Update(elapsedTime);
    
     if (!is_my_player) {

         // 위치 동기화
         OpponentMoveSync(elapsedTime);

         // 회전 동기화 (Yaw / Pitch)
         OpponentRotateSync(elapsedTime);
     }
}

void CPlayer::OpponentMoveSync(float elapsedTime)
{
    XMFLOAT3 serverPos{ dest_info.x, dest_info.y, dest_info.z };
    XMFLOAT3 clientPos = position;

    XMFLOAT3 toTarget = Vector3::Subtract(serverPos, clientPos);
    float dist = Vector3::Length(toTarget);

    const float DAMPING = 10.0f;   // 클수록 더 빨리 따라가고 더 빨리 멈춤

    // 1. 너무 멀면 스냅 (순간이동 보정)
    if (dist > 3.0f * speed)
    {
        SetPosition(serverPos);
        velocity = XMFLOAT3(0, 0, 0);
        SetYaw(dest_info.yaw);
        return;
    }

    // 2. 서버가 WALK 상태고 아직 충분히 멀면 → 따라가기
    if (dest_info.state == PLAYER_STATE::WALK && dist > 0.05f * speed)
    {
        // 방향 벡터
        XMFLOAT3 dir = Vector3::Normalize(toTarget);

        // 목표 속도 (초당 이동량 개념)
        XMFLOAT3 desiredVel = Vector3::ScalarProduct(dir, speed);

        // velocity를 desiredVel 쪽으로 부드럽게 보간
        float lerpFactor = std::min(1.0f, DAMPING * elapsedTime);
        velocity = Vector3::Add(
            velocity,
            Vector3::ScalarProduct(
                Vector3::Subtract(desiredVel, velocity),
                lerpFactor,
                false
            )
        );

        // 위치 이동 (speed 다시 곱하지 않음!!)
        XMFLOAT3 frameMove = Vector3::ScalarProduct(velocity, elapsedTime, false);
        position = Vector3::Add(position, frameMove);

        // 방향 동기화
        SetYaw(dest_info.yaw);
    }
    else
    {
        // 3. 서버가 IDLE 이거나 거의 도착 → 감속하면서 정지
        float decay = std::max(0.0f, 1.0f - DAMPING * elapsedTime);
        velocity = Vector3::ScalarProduct(velocity, decay, false);

        XMFLOAT3 frameMove = Vector3::ScalarProduct(velocity, elapsedTime, false);
        position = Vector3::Add(position, frameMove);
    }
}

void CPlayer::OpponentRotateSync(float elapsedTime)
{
    float targetYaw = dest_info.yaw;
    float targetPitch = dest_info.pitch;

    // 현재 각도 (멤버 변수)
    float curYaw = yaw;
    float curPitch = pitch;

    // Yaw 360도 경계 보정
    float deltaYaw = targetYaw - curYaw;
    if (deltaYaw > 180.f)  deltaYaw -= 360.f;
    if (deltaYaw < -180.f) deltaYaw += 360.f;

    float deltaPitch = targetPitch - curPitch;

    // 회전 보간 속도
    const float rotSpeed = 8.0f;

    // 프레임 보간
    yaw += deltaYaw * rotSpeed * elapsedTime;
    pitch += deltaPitch * rotSpeed * elapsedTime;

    // Pitch 제한 (필수)
    pitch = std::clamp(pitch, -89.9f, 89.9f);

    // 회전 적용
    SetYawPitch(yaw, pitch);
    UpdateWorldMatrix();
}
