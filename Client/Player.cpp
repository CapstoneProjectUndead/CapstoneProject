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
     CCharacter::Update(elapsedTime);
    
     if (!is_my_player) {

         // 상대 위치 동기화
         OpponentMoveSyncByInterpolation(elapsedTime);

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

    float speed{};
    if (auto move = GetComponent<CMovementComponent>())
        speed = move->GetSpeed();
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

void CPlayer::OpponentMoveSyncByInterpolation(float dt)
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

        // [핵심] 다음 프레임에 가야 할 목표 위치를 먼저 계산
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        if (dt > 0.0f)
        {
            // 현재 position에서 nextPos로 가기 위한 '속도'를 먼저 구함
            // 이때 position은 아직 업데이트되기 전의 "현재 위치" 이다.
            XMFLOAT3 moveDelta = Vector3::Subtract(nextPos, position);
            velocity = Vector3::ScalarProduct(moveDelta, 1.0f / dt, false);

            // 속도가 일정 수준 이상일 때만 WALK 상태로 전환
            float currentSpeed = Vector3::Length(velocity);
            if (currentSpeed > 0.05f) { // 0.1이 너무 크면 0.05 정도로 낮춰볼 것
                state = PLAYER_STATE::WALK;
            }
            else {
                state = PLAYER_STATE::IDLE;
                velocity = { 0, 0, 0 };
            }
        }

        // 속도 계산이 끝난 후에 드디어 위치를 옮김
        SetPosition(nextPos);
    }

    // 4. 장부 정리
    while (interpolation_deq.size() > 2 && interpolation_deq[1].serverTimestamp < targetServerTime)
    {
        interpolation_deq.pop_front();
    }
}
