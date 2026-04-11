#include "stdafx.h"
#include "Player.h"
#include "NetworkManager.h"
#include "Movement.h"
#include "NetworkClockManager.h"
#include "JitterMeasurer.h"
#include "Animator.h"
#include "Material.h"
#include "MeshRenderer.h"

#undef min
#undef max

// Player
CPlayer::CPlayer()
	: CCharacter(OBJECT_TYPE::PLAYER)
    , room_id(-1)
{
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CPlayer::Update(float elapsedTime)
{
    UpdateBuffs(elapsedTime);
    PreUpdate(elapsedTime);

    CCharacter::Update(elapsedTime);
}

void CPlayer::AddBuff(const Buff& buff)
{
    buffs.push_back(buff);
}

void CPlayer::UpdateBuffs(float elapsedTime)
{
    for (auto& buff : buffs)
        buff.duration -= elapsedTime;

    buffs.erase(
        std::remove_if(buffs.begin(), buffs.end(), [](const Buff& b) { return b.duration <= 0.f; }),
        buffs.end());
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

void CPlayer::OnCollect(IRenderer* renderer)
{
    CObject::OnCollect(renderer);

    auto animator = GetComponent<CAnimatorComponent>();
    if (animator) {
        if (equipped_item_id != 0)
            animator->RenderSocketModel(CAnimatorComponent::HAND_R, equipped_item_id);
    }
}

void CPlayer::RecordOpponentFrameHistory(const OpponentFrameHistory& state)
{
    interpolation_deq.push_back(state);

    if (interpolation_deq.size() > RENDER_BUFFER_MAX_SIZE)
        interpolation_deq.pop_front();
}

XMVECTOR CPlayer::GetHeadPosition() const
{
    auto* animator = GetComponent<CAnimatorComponent>();
    if (!animator) return XMLoadFloat3(&position);

    return animator->GetHeadPosition();
}

void CPlayer::OpponentMoveSyncByInterpolation(float elapsedTime)
{
    // 데이터가 2개 미만이면 보간 불가능
    if (interpolation_deq.size() < 2) 
        return;

    // ---------------------------------------------------------
    // 1. 시간 동기화 및 타겟 시간 설정
    // ---------------------------------------------------------
    float serverNow = CNetworkClockManager::GetInstance().GetServerNow();

    auto measurer = CNetworkManager::GetInstance().GetJitterMeasurer();

    // 1. 목표 딜레이 계산 (기존 로직)
        // Jitter가 튈 때마다 이 값은 미친 듯이 널뛰기를 합니다.
#ifdef GENERATE_LAG
    float rawDelay = (measurer->GetAverageInterval() * 2.0f) + (measurer->GetCurrentJitter() * 25.0f);
#else
    float rawDelay = (measurer->GetAverageInterval() * 1.5f) + (measurer->GetCurrentJitter() * 5.0f);
#endif 
    float targetDelay = std::clamp(rawDelay, 0.05f, 2.0f);

    // 60틱(16ms) 기준이므로 최소 딜레이를 조금 여유 있게 잡음
    smoothed_delay = std::lerp(smoothed_delay, targetDelay, 0.01f);

    // 기본 타겟 시간
    float targetServerTime = serverNow - smoothed_delay;

    // ---------------------------------------------------------
    // 2. 시간 축 보정 (Time Warp & Clamp)
    // ---------------------------------------------------------

    // [Time Warp] 렉이 풀려서 데이터가 몰렸거나 시계가 너무 뒤처진 경우
    // 버퍼가 1초(60개) 크기이므로, 0.7초 이상 차이나면 최신으로 강제 견인
    if (interpolation_deq.back().server_timestamp - targetServerTime > 0.7f) {
        targetServerTime = interpolation_deq.back().server_timestamp - smoothed_delay;
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

        // 4-3. 텔레포트 체크 (오차가 너무 클 때만)
        float distToTarget = Vector3::Length(Vector3::Subtract(nextPos, position));
        if (distToTarget > 5.0f) {
            SetPosition(nextPos);
            // 텔레포트 시엔 잠시 멈춤 상태로 두거나, 서버 상태를 따르거나 선택
            // (보통 멀리서 텔포되면 잠깐 IDLE인 게 자연스러울 수 있음)
            state = PLAYER_STATE::IDLE;
        }
        else {
            // [보간 이동]
            // Frame A와 Frame B 사이의 실제 이동 거리 계산
            // (서버가 보내준 두 점 사이의 거리가 얼마나 되는가?)
            float intervalDist = Vector3::Length(Vector3::Subtract(frameB->position, frameA->position));

            // 두 패킷 사이의 거리가 1cm 미만(0.01f)이면 '사실상 멈춤'으로 간주
            if (intervalDist < 0.01f) {

                state = PLAYER_STATE::IDLE;

                if (frameA->state == PLAYER_STATE::WALK || frameA->state == PLAYER_STATE::RUN)
                    state = frameA->state;
                if (frameB->state == PLAYER_STATE::WALK || frameB->state == PLAYER_STATE::RUN)
                    state = frameB->state;

                nextPos = position;
            }
            else {
                if (frameA->state == PLAYER_STATE::RUN)
                    state = PLAYER_STATE::RUN;
                else
                    state = PLAYER_STATE::WALK;
            }

            // 위치 이동
            SetPosition(nextPos);
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
    SetYawPitch(yaw, 0.0f);
    UpdateWorldMatrix();
}

void CPlayer::ChangeModelSet(int setIndex)
{
    for (int i = 0; i < eartail_parts.size(); ++i) {
        bool active = (i == setIndex);
        
        body_materials[i]->SetEnable(active);

        for (auto& mesh : eartail_parts[i]) {
            mesh->SetEnable(active);
        }
    }
}

void CPlayer::ChangeEyes(int index)
{
    for (int i = 0; i < eyes_material.size(); ++i) {
        eyes_material[i]->SetEnable(i == index);
    }
}

void CPlayer::ChangeMouth(int index)
{
    for (int i = 0; i < mouth_material.size(); ++i) {
        mouth_material[i]->SetEnable(i == index);
    }
}
