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

         // 상대 위치 동기화
         OpponentMoveSync(elapsedTime);

         // 회전 동기화 (Yaw / Pitch)
         OpponentRotateSync(elapsedTime);
     }
}

void CPlayer::OpponentMoveSync(const float elapsedTime)
{
    XMFLOAT3 serverPos{ dest_info.x, dest_info.y, dest_info.z };
    XMFLOAT3 clientPos = position;

    XMFLOAT3 toTarget = Vector3::Subtract(serverPos, clientPos);
    float dist = Vector3::Length(toTarget);

    const float SNAP_DIST = 3.0f * speed;
    const float ARRIVE_DIST = 0.01f * speed;

    // 1. 너무 멀면 스냅
    if (dist > SNAP_DIST)
    {
        SetPosition(serverPos);
        velocity = XMFLOAT3(0, 0, 0);
        SetYaw(dest_info.yaw);
        return;
    }

    // 2. 서버가 움직이는 중이면 → 정확히 "도착 시간" 기반으로 이동
    if (dest_info.state == PLAYER_STATE::WALK && dist > ARRIVE_DIST)
    {
        // 서버 패킷 주기 = 1/60
        constexpr float SERVER_TICK = 1.0f / 60.0f;

        // 이번 프레임에 가야 할 비율
        float alpha = std::min(1.0f, elapsedTime / SERVER_TICK);

        // 위치 직접 보간 (절대 뒤처지지 않음)
        position = Vector3::Add(
            position,
            Vector3::ScalarProduct(toTarget, alpha, false)
        );

        // 가짜 velocity (애니메이션용)
        velocity = Vector3::ScalarProduct(
            Vector3::Normalize(toTarget),
            speed,
            false
        );

        SetYaw(dest_info.yaw);
    }
    else
    {
        // 3. 거의 도착 → 정확히 서버 위치로 스냅
        SetPosition(serverPos);
        velocity = XMFLOAT3(0, 0, 0);
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
