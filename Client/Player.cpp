#include "stdafx.h"
#include "Player.h"

#undef min
#undef max

extern HWND ghWnd;

// Player
CPlayer::CPlayer()
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CPlayer::Update(float elapsedTime)
{
     if (is_my_player)
        return;
    
     // 위치 동기화
     OpponentMoveSync(elapsedTime);
     
     // 회전 동기화 (Yaw / Pitch)
     OpponentRotateSync(elapsedTime);
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
}

void CPlayer::OpponentMoveSync(float elapsedTime)
{
    XMFLOAT3 serverPos{ dest_info.x, dest_info.y, dest_info.z };
    XMFLOAT3 clientPos = position;

    XMFLOAT3 toTarget = Vector3::Subtract(serverPos, clientPos);
    float dist = Vector3::Length(toTarget);

    const float FOLLOW_SPEED = 1.0f;   // 네 캐릭터 이동속도
    const float DAMPING = 10.0f;       // 클수록 더 빨리 멈춤

    // 1. 너무 멀면 스냅
    if (dist > 3.0f)
    {
        SetPosition(serverPos);
        velocity = XMFLOAT3(0, 0, 0);
        return;
    }

    // 2. 상대가 WALK 상태일 때만 따라간다
    if (dest_info.state == PLAYER_STATE::WALK && dist > 0.05f)
    {
        // 방향
        XMFLOAT3 dir = Vector3::Normalize(toTarget);

        // 목표 속도
        XMFLOAT3 desiredVel = Vector3::ScalarProduct(dir, FOLLOW_SPEED);

        // velocity 보간 
        velocity = Vector3::Add(
            velocity,
            Vector3::ScalarProduct(
                Vector3::Subtract(desiredVel, velocity),
                std::min(1.0f, DAMPING * elapsedTime),
                false
            )
        );

        // 위치 이동
        XMFLOAT3 frameMove = Vector3::ScalarProduct(velocity, elapsedTime, false);
        position = Vector3::Add(position, frameMove);

        // 방향 동기화
        SetYaw(dest_info.yaw);
    }
    else
    {
        // 서버가 IDLE 이거나 거의 도착 → 부드럽게 정지
        velocity = Vector3::ScalarProduct(
            velocity,
            std::max(0.0f, 1.0f - DAMPING * elapsedTime),
            false
        );

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
