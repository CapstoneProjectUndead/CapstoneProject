#include "stdafx.h"
#include "Player.h"
#include "NetworkManager.h"
#include "Movement.h"

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
    PreUpdate(elapsedTime);

    CCharacter::Update(elapsedTime);
}

void CPlayer::PreUpdate(float elapsedTime)
{
    if (!is_my_player) {

        // 상대 위치 동기화
        OpponentMoveSyncByInterpolation(elapsedTime);

        // 상대 회전 동기화 (Yaw / Pitch)
        OpponentRotateSync(elapsedTime);
    }
}

void CPlayer::OpponentMoveSyncByInterpolation(float elapsedTime)
{
    if (interpolation_deq.size() < 2) return;

    // 1. 타겟 시간 및 딜레이 계산 
    auto measurer = CNetworkManager::GetInstance().GetJitterMeasurer();
    float jitter = measurer->GetCurrentJitter();
    float avgInterval = measurer->GetAverageInterval();

    float adaptiveDelay = (avgInterval * 1.5f) + (jitter * 5.0f);
    float interpolationDelay = std::clamp(adaptiveDelay, 0.033f, 1.0f);
    float targetServerTime = interpolation_deq.back().serverTimestamp - interpolationDelay;

    // 2. 보간 구간 찾기
    OpponentState* frameA = nullptr;
    OpponentState* frameB = nullptr;

    for (size_t i = 0; i < interpolation_deq.size() - 1; ++i)
    {
        if (interpolation_deq[i].serverTimestamp <= targetServerTime &&
            interpolation_deq[i + 1].serverTimestamp >= targetServerTime)
        {
            frameA = &interpolation_deq[i];
            frameB = &interpolation_deq[i + 1];
            break;
        }
    }

    // 3. 보간 실행
    if (frameA && frameB)
    {
        float timeDiff = frameB->serverTimestamp - frameA->serverTimestamp;
        float alpha = (timeDiff > 0.0f) ? (targetServerTime - frameA->serverTimestamp) / timeDiff : 0.0f;

        // [핵심] 다음 프레임에 가야 할 목표 위치
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        // [추가된 검증 로직] 
        // Frame A와 Frame B 사이의 거리가 너무 작으면(사실상 제자리), 
        // 아예 속도 계산을 하지 않고 강제로 정지시킨다.
        float intervalDist = Vector3::Length(Vector3::Subtract(frameB->position, frameA->position));

        // 0.01f = 1cm. 두 패킷 사이의 거리가 1cm 미만이면 멈춘 것으로 간주
        if (intervalDist < 0.01f)
        {
            state = PLAYER_STATE::IDLE;
            velocity = { 0, 0, 0 };
        }
        else if (elapsedTime > 0.0f)
        {
            // 움직임이 확실할 때만 정밀 속도 계산 수행
            XMFLOAT3 moveDelta = Vector3::Subtract(nextPos, position);
            velocity = Vector3::ScalarProduct(moveDelta, 1.0f / elapsedTime, false);

            float currentSpeed = Vector3::Length(velocity);

            // 임계값을 0.05보다 약간 여유 있게 0.1로 올리거나 유지
            if (currentSpeed > 0.05f) {
                state = PLAYER_STATE::WALK;
            }
            else {
                state = PLAYER_STATE::IDLE;
                velocity = { 0, 0, 0 };
            }
        }
    }

    // 4. 장부 정리
    while (interpolation_deq.size() > 2 && interpolation_deq[1].serverTimestamp < targetServerTime)
    {
        interpolation_deq.pop_front();
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