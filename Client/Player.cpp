#include "stdafx.h"
#include "Player.h"
#include "NetworkManager.h"

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
         //OpponentMoveSync(elapsedTime);
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

void CPlayer::OpponentMoveSyncByInterpolation(float dt)
{
    // 1. 최소 데이터 확인 (보간을 위해 최소 2개의 지점이 필요)
    if (interpolation_deq.size() < 2) return;

    // [안전장치] 너무 멀리 떨어져 있으면 보간이고 뭐고 일단 순간이동 (기존 SNAP 로직)
    XMFLOAT3 latestServerPos = interpolation_deq.back().position;
    float distToLatest = Vector3::Length(Vector3::Subtract(latestServerPos, position));
    const float SNAP_DIST = 5.0f * speed; // 거리 기준은 프로젝트에 맞게 조절

    if (distToLatest > SNAP_DIST)
    {
        SetPosition(latestServerPos);
        interpolation_deq.clear(); // 버퍼도 비워서 새로 시작하게 함
        return;
    }

    // 2. 타겟 서버 시간 계산 (최신 패킷 시간 기준 100ms 전 과거)
    float jitter = CNetworkManager::GetInstance().GetJitterMeasurer()->GetCurrentJitter();

    float interpolationDelay = std::clamp(jitter * 0.3f, 0.066f, 0.250f);
    float targetServerTime = interpolation_deq.back().serverTimestamp - interpolationDelay;

    // 3. 타겟 시간을 포함하는 구간(A-B) 찾기
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

    // 4. 보간 실행
    if (frameA && frameB)
    {
        float timeDiff = frameB->serverTimestamp - frameA->serverTimestamp;
        float alpha = 0.0f;
        if (timeDiff > 0.0f)
            alpha = (targetServerTime - frameA->serverTimestamp) / timeDiff;

        // [핵심] 선형 보간으로 위치 결정
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        // 애니메이션을 위한 속도 계산 (현재 위치와 다음 보간 위치의 차이)
        velocity = Vector3::ScalarProduct(Vector3::Subtract(nextPos, position), 1.0f / dt, false);

        SetPosition(nextPos);
        SetYaw(yaw); // 방향도 서버 데이터에 맞게 회전
    }
    else if (targetServerTime > interpolation_deq.back().serverTimestamp)
    {
        // 만약 타겟 시간이 최신 패킷보다 더 미래라면 (네트워크 지연으로 데이터 부족)
        // 마지막 알려진 서버 위치로 부드럽게 멈춤 (기존 로직의 도착 스냅과 유사)
        SetPosition(latestServerPos);
        velocity = { 0, 0, 0 };
    }

    // 5. 너무 오래된 장부 정리
    while (interpolation_deq.size() > 2 && interpolation_deq[1].serverTimestamp < targetServerTime)
    {
        interpolation_deq.pop_front();
    }
}
