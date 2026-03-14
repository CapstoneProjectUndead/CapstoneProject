#include "stdafx.h"
#include "Monster.h"
#include "NetworkManager.h"
#include "NetworkClockManager.h"
#include "Animator.h"
#include "MyPlayer.h"
#include "SceneManager.h"

CMonster::CMonster(MON_TYPE type)
	: CCharacter(OBJECT_TYPE::MONSTER)
	, origin_position{}
	, monster_type(type)
	, AI_state(AI_STATE::MONSTER_IDLE)
    , idle_timer(0.0f)
    , patrol_timer(0.0f)
    , attack_timer(0.0f)
    , turn_timer(0.0f)
{
}

CMonster::~CMonster()
{
}

void CMonster::Update(float elapsedTime)
{
    // (싱글 모드)
    if (g_is_single) {
        UpdateSingle(elapsedTime);
    }
    else {
        UpdateMulti(elapsedTime);
    }
}

void CMonster::UpdateSingle(float elapsedTime)
{
    // 물리 엔진이 돌기 전의 현재 Y 좌표를 기억합니다.
    float beforeY = position.y;

    CCharacter::Update(elapsedTime);

    // (싱글 환경) 몬스터 떨림 문제 방지 코드
    if (g_is_single && is_grounded && std::abs(velocity.y) < 0.1f) {

        // 물리 엔진이 방금 뱉어낸 Y값과 원래 Y값의 차이 계산
        float yDiff = std::abs(position.y - beforeY);

        // 오차가 0 초과 0.05f 미만이라면 -> 이건 이동이 아니라 EPA 충돌 보정에 의한 '떨림'이다!
        if (yDiff > 0.0f && yDiff < 0.05f) {
            position.y = Math::Lerp(beforeY, position.y, 0.2f); // 0.2f는 흡수 강도 (조절 가능)
        }
    }
}

void CMonster::UpdateMulti(float elapsedTime)
{
    // 멀티 플레이일 경우에만 실행!
    if (!g_is_single) {
        MonsterMoveSyncByInterpolation(elapsedTime);
    }

    CCharacter::Update(elapsedTime);
}

void CMonster::RecordMonsterFrameHistory(const MonsterFrameHistory& state)
{
    interpolation_deq.push_back(state);

    if (interpolation_deq.size() > RENDER_BUFFER_MAX_SIZE)
        interpolation_deq.pop_front();
}

void CMonster::MonsterMoveSyncByInterpolation(float elapsedTime)
{
    // 데이터가 2개 미만이면 보간 불가능
    if (interpolation_deq.size() < 2)
        return;

    // ---------------------------------------------------------
    // 1. 시간 동기화 및 타겟 시간 설정
    // ---------------------------------------------------------
    float serverNow = CNetworkClockManager::GetInstance().GetServerNow();

    auto measurer = CNetworkManager::GetInstance().GetJitterMeasurer();

    float rawDelay = (measurer->GetAverageInterval() * 1.5f) + (measurer->GetCurrentJitter() * 5.0f);
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

    MonsterFrameHistory* frameA = nullptr;
    MonsterFrameHistory* frameB = nullptr;

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

        // 알파값 구하기
        float timeDiff = frameB->server_timestamp - frameA->server_timestamp;
        float alpha = (timeDiff > 0.0001f) ? (targetServerTime - frameA->server_timestamp) / timeDiff : 0.0f;

        // 목표 위치, 회전, 상태 계산
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        // 회전값은 바로 적용, 어떤 애니메이션을 재생할지에 대한 상태값도 바로 적용!
        SetYaw(frameB->yaw);
        SetPitch(frameB->pitch);
        AI_state = frameB->AI_state;

        if (AI_state == AI_STATE::MONSTER_IDLE) {

            // 서버가 보내준 미래의 Y값(frameB)과 현재 내 Y값의 차이를 구함
            float yDiff = std::abs(frameB->position.y - position.y);

            // 오차가 0.05f(5cm) 미만이라면, 이건 이동이 아니라 물리 엔진의 '떨림'으로 간주!
            if (yDiff < 0.05f) {
                nextPos.y = position.y; // Y축은 덜덜거리지 않게 현재 위치로 꽉 잡아둠
            }
        }
        // -------------------------------------------------------------

        // 텔레포트 체크 및 이동 적용
        float distToTarget = Vector3::Length(Vector3::Subtract(nextPos, position));

        if (distToTarget > 5.0f) {
            SetPosition(nextPos);
        }
        else{
            float lerpSpeed = 15.0f;

            // 프레임 속도에 상관없이 일정한 속도로 보간하게 만듭니다.
            float smoothFactor = 1.0f - std::exp(-lerpSpeed * elapsedTime);
            nextPos.y = Math::Lerp(position.y, nextPos.y, smoothFactor);

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

std::shared_ptr<CPlayer> CMonster::FindNearestPlayer()
{
    CScene* currentScene = CSceneManager::GetInstance().GetActiveScene();
    assert(currentScene->GetSceneType() == current_scene_type);
    auto player = currentScene->GetMyPlayer();

    return player;
}