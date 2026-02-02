#include "stdafx.h"
#include "Player.h"
#include "NetworkManager.h"
#include "Movement.h"
#include "NetworkClockManager.h"

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

void CPlayer::RecordOpponentFrameHistory(const OpponentFrameHistory& state)
{
    interpolation_deq.push_back(state);

    if (interpolation_deq.size() > RENDER_BUFFER_MAX_SIZE)
        interpolation_deq.pop_front();
}

void CPlayer::OpponentMoveSyncByInterpolation(float elapsedTime)
{
    // 데이터가 2개 미만이면 보간 불가능
    if (interpolation_deq.size() < 2) return;

    // ---------------------------------------------------------
    // 1. 시간 동기화 및 타겟 시간 설정
    // ---------------------------------------------------------
    float serverNow = CNetworkClockManager::GetInstance().GetServerNow();

    auto measurer = CNetworkManager::GetInstance().GetJitterMeasurer();
    float adaptiveDelay = (measurer->GetAverageInterval() * 1.5f) + (measurer->GetCurrentJitter() * 5.0f);

    // 60틱(16ms) 기준이므로 최소 딜레이를 조금 여유 있게 잡음
    float interpolationDelay = std::clamp(adaptiveDelay, 0.05f, 1.0f);

    // 기본 타겟 시간
    float targetServerTime = serverNow - interpolationDelay;

    // ---------------------------------------------------------
    // 2. 시간 축 보정 (Time Warp & Clamp)
    // ---------------------------------------------------------

    // [Time Warp] 렉이 풀려서 데이터가 몰렸거나 시계가 너무 뒤처진 경우
    // 버퍼가 1초(60개) 크기이므로, 0.7초 이상 차이나면 최신으로 강제 견인
    if (interpolation_deq.back().server_timestamp - targetServerTime > 0.7f) {
        targetServerTime = interpolation_deq.back().server_timestamp - interpolationDelay;
    }

    // [안전장치] 미래 데이터 방지 (데이터 부족 시 대기)
    if (targetServerTime > interpolation_deq.back().server_timestamp) {
        targetServerTime = interpolation_deq.back().server_timestamp;
    }

    // [안전장치] 과거 데이터 방지 (버퍼 범위 이탈 방지)
    if (targetServerTime < interpolation_deq.front().server_timestamp) {
        targetServerTime = interpolation_deq.front().server_timestamp;
    }

    // ---------------------------------------------------------
    // 3. 보간 구간 검색 (Frame A, Frame B)
    // ---------------------------------------------------------

    OpponentFrameHistory* frameA = nullptr;
    OpponentFrameHistory* frameB = nullptr;

    for (size_t i = 0; i < interpolation_deq.size() - 1; ++i) {
        if (interpolation_deq[i].server_timestamp <= targetServerTime &&
            interpolation_deq[i + 1].server_timestamp >= targetServerTime) {
            frameA = &interpolation_deq[i];
            frameB = &interpolation_deq[i + 1];
            break;
        }
    }

    // ---------------------------------------------------------
    // 4. 보간 및 이동 처리
    // ---------------------------------------------------------

    if (frameA && frameB) {

        // 4-1. 알파값 구하기
        float timeDiff = frameB->server_timestamp - frameA->server_timestamp;
        float alpha = (timeDiff > 0.0001f) ? (targetServerTime - frameA->server_timestamp) / timeDiff : 0.0f;

        // 4-2. 목표 위치 계산
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        // [Spatial Teleport] 디버깅 해제 직후 등, 거리가 너무 멀면 즉시 이동
        float distToTarget = Vector3::Length(Vector3::Subtract(nextPos, position));
        if (distToTarget > 5.0f) {
            SetPosition(nextPos);
            velocity = { 0.0f, 0.0f, 0.0f };
            state = PLAYER_STATE::IDLE;
        }
        else {
            // [Movement] 미세 움직임 필터링 및 속도 계산
            float intervalDist = Vector3::Length(Vector3::Subtract(frameB->position, frameA->position));

            // 두 패킷 사이 거리가 1cm 미만이면 정지로 간주
            if (intervalDist < 0.01f) {
                state = PLAYER_STATE::IDLE;
                velocity = { 0.0f, 0.0f, 0.0f };
                // 정지 상태라도 위치 보정은 해야 함 (미세한 떨림 방지 위해 nextPos로 이동)
                SetPosition(nextPos);
            }
            else if (elapsedTime > 0.0f) {
                // 속도 역산
                XMFLOAT3 moveDelta = Vector3::Subtract(nextPos, position);
                velocity = Vector3::ScalarProduct(moveDelta, 1.0f / elapsedTime, false);

                float currentSpeed = Vector3::Length(velocity);

                if (currentSpeed > 0.05f) {
                    state = PLAYER_STATE::WALK;
                }
                else {
                    state = PLAYER_STATE::IDLE;
                    velocity = { 0.0f, 0.0f, 0.0f };
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 5. 장부 정리 (Buffer Management)
    // ---------------------------------------------------------
    // targetServerTime보다 확실히 과거인 데이터는 삭제하되, 보간을 위해 최소 2개는 남김

    while (interpolation_deq.size() > 2 && interpolation_deq[1].server_timestamp < targetServerTime) {
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