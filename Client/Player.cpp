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

void CPlayer::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
    // DEAD: 상대방 모델 렌더링 안 함 (관전 모드)
    if (state == PLAYER_STATE::DEAD)
        return;

    CObject::OnCollect(renderers);

    auto animator = GetComponent<CAnimatorComponent>();
    if (animator) {
        if (is_dowsing) {
            animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_R, NULL, "dowsing_rod_0307");
            animator->RenderSocketModel(CAnimatorComponent::HAND_ROD_L, NULL, "dowsing_rod_0307");
        }
        else if (!is_dowsing && equipped_item_id != 0)
            animator->RenderSocketModel(CAnimatorComponent::HAND_R, equipped_item_id);
    }
}

void CPlayer::OnAttack()
{
    auto* animator = GetComponent<CAnimatorComponent>();

    std::string attackClip = animator->GetAttackClipByItem(GetEquippedItemId());

    animator->PlayAction(attackClip);
}

// One-Euro 필터 알파 계산: cutoff(Hz)이 낮을수록 더 부드럽게(많이 평활), dt는 프레임 간격
static float EuroAlpha(float cutoff, float dt)
{
    constexpr float kPi = 3.14159265358979f;
    float tau = 1.0f / (2.0f * kPi * cutoff);
    return 1.0f / (1.0f + tau / dt);
}

void CPlayer::RecordOpponentFrameHistory(const OpponentFrameHistory& state)
{
    OpponentFrameHistory filtered = state;

    // [One-Euro 필터] 서버 충돌계산이 메시 바닥에서 매 틱 좌표를 흔든다(YDiag로 Y 진폭 ~2.7mm,
    // 버스트 3~7cm 측정). 원격 플레이어는 클라에 물리·예측이 없는 표시 전용 객체이므로, 받은
    // 좌표를 보간 입력 단계에서 평활한다. 이진 데드밴드와 달리 One-Euro는 "느리면 강하게,
    // 빠르면 약하게" 적응 평활하므로 떨림은 잡고 빠른 이동엔 렉(고무줄)이 안 생긴다.
    // ===== 튜닝 노브 =====
    //  kMinCutoff : 정지/저속 시 cutoff(Hz). 낮출수록 더 부드러움(떨림↓), 너무 낮으면 반응 둔해짐.
    //  kBeta      : 속도에 따라 cutoff를 올리는 정도. 높일수록 빠른 이동에서 렉↓(단 떨림 살짝↑).
    constexpr float kMinCutoff = 0.3f;   // Hz
    constexpr float kBeta      = 0.2f;
    constexpr float kDCutoff   = 1.0f;   // Hz (미분값 평활용)

    if (!euro_init) {
        // 첫 샘플: 필터 초기화
        euro_init    = true;
        euro_last_ts = state.server_timestamp;
        euro_x_prev  = state.position;
        euro_x_hat   = state.position;
        euro_dx_hat  = XMFLOAT3(0.0f, 0.0f, 0.0f);
    }
    else {
        // 텔레포트/리스폰: 큰 점프는 평활하지 말고 즉시 스냅(필터 리셋)
        float jump = Vector3::Length(Vector3::Subtract(state.position, euro_x_hat));
        if (jump > 2.0f) {
            euro_last_ts = state.server_timestamp;
            euro_x_prev  = state.position;
            euro_x_hat   = state.position;
            euro_dx_hat  = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }
        else {
            float dt = state.server_timestamp - euro_last_ts;
            if (dt < 0.0001f) dt = 1.0f / 60.0f; // 동일 타임스탬프 안전장치
            euro_last_ts = state.server_timestamp;

            auto filterAxis = [&](float x, float& xPrev, float& xHat, float& dxHat) -> float {
                // 1. 미분(속도) 추정 후 미분값 평활
                float dx  = (x - xPrev) / dt;
                float aD  = EuroAlpha(kDCutoff, dt);
                float edx = aD * dx + (1.0f - aD) * dxHat;
                dxHat = edx;
                // 2. 속도가 클수록 cutoff를 올려 평활 약화(렉 방지)
                float cutoff = kMinCutoff + kBeta * std::abs(edx);
                float aX = EuroAlpha(cutoff, dt);
                float out = aX * x + (1.0f - aX) * xHat;
                xHat  = out;
                xPrev = x;
                return out;
            };

            filtered.position.x = filterAxis(state.position.x, euro_x_prev.x, euro_x_hat.x, euro_dx_hat.x);
            filtered.position.y = filterAxis(state.position.y, euro_x_prev.y, euro_x_hat.y, euro_dx_hat.y);
            filtered.position.z = filterAxis(state.position.z, euro_x_prev.z, euro_x_hat.z, euro_dx_hat.z);
        }
    }

    interpolation_deq.push_back(filtered);

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
            // [빈사/사망 최우선] 빈사·사망은 다른 어떤 행동(JUMP/DIG/ATTACK)보다 우선.
            // 빈사 진입 직전 프레임(예: 공격 중 사망)이 보간 윈도우에 잡혀도 JUMP/DIG/ATTACK
            // 보존 분기가 먼저 매칭되어 ALMOST_DEAD가 묻히는 것을 방지.
            if (frameA->state == PLAYER_STATE::ALMOST_DEAD || frameB->state == PLAYER_STATE::ALMOST_DEAD) {
                state = PLAYER_STATE::ALMOST_DEAD;
            }
            else if (frameA->state == PLAYER_STATE::DEAD || frameB->state == PLAYER_STATE::DEAD) {
                state = PLAYER_STATE::DEAD;
            }
            // [점프 우선] 서버가 grounded_timer 디바운스로 깔끔하게 결정한 JUMP state는 그대로 보존
            else if (frameA->state == PLAYER_STATE::JUMP || frameB->state == PLAYER_STATE::JUMP) {
                state = PLAYER_STATE::JUMP;
            }
            // [채굴 우선] 서버가 dig_timer로 결정한 DIG state 보존
            else if (frameA->state == PLAYER_STATE::DIG || frameB->state == PLAYER_STATE::DIG) {
                state = PLAYER_STATE::DIG;
            }
            // [공격 우선] 서버가 spray_attack_timer로 결정한 ATTACK state 보존
            // PlayAction은 Handle_S_Move_Player에서 패킷 수신 즉시 호출 (보간 지연 없음)
            else if (frameA->state == PLAYER_STATE::ATTACK || frameB->state == PLAYER_STATE::ATTACK) {
                state = PLAYER_STATE::ATTACK;
            }
            else {
                // [보간 이동] 서버가 보낸 상태(WALK/RUN/IDLE)를 그대로 사용한다.
                // 위치 간격(intervalDist)으로 상태를 재분류하면, 서버가 가끔 보내는
                // near-duplicate 프레임에서 RUN↔IDLE이 깜빡여 달리기 애니메이션이 부들거린다.
                state = frameB->state;
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

void CPlayer::SetEquippedItemId(uint16 id)
{
    equipped_item_id = id;

    if (auto animator = GetComponent<CAnimatorComponent>()) {
        animator->OnChangeEquippedItem(id);
    }
}

void CPlayer::ChangeModelSet(int setIndex)
{
    model_type_idx = setIndex;

    // UI 프로필 이미지 매핑
    player_image = static_cast<PLAYER_IMAGE>((setIndex % 6) + 1);

    // 귀/꼬리 메시 활성화/비활성화 제어 (0,3 -> Dog / 1,4 -> Cat / 2,5 -> Bunny)
    int meshGroupIndex = setIndex % 3;
    for (int i = 0; i < eartail_parts.size(); ++i) {
        bool active = (i == meshGroupIndex);
        for (auto& mesh : eartail_parts[i]) {
            mesh->SetEnable(active);
        }
    }
}