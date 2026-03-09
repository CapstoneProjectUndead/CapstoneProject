#include "stdafx.h"
#include "Monster.h"
#include "NetworkManager.h"
#include "NetworkClockManager.h"
#include "Animator.h"

CMonster::CMonster(MON_TYPE type)
	: CCharacter(OBJECT_TYPE::MONSTER)
	, origin_position{}
	, monster_type(type)
	, AI_state(AI_STATE::MONSTER_IDLE)
{
}

CMonster::~CMonster()
{
}

void CMonster::Update(float elapsedTime)
{
    MonsterMoveSyncByInterpolation(elapsedTime);

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

        // 4-1. 알파값 구하기
        float timeDiff = frameB->server_timestamp - frameA->server_timestamp;
        float alpha = (timeDiff > 0.0001f) ? (targetServerTime - frameA->server_timestamp) / timeDiff : 0.0f;

        // 4-2. 목표 위치 계산
        XMFLOAT3 nextPos = Vector3::Lerp(frameA->position, frameB->position, alpha);

        // 4-3. 텔레포트 체크 (오차가 너무 클 때만)
        float distToTarget = Vector3::Length(Vector3::Subtract(nextPos, position));
        if (distToTarget > 5.0f) {
            SetPosition(nextPos);
            AI_state = AI_STATE::MONSTER_IDLE;
        }
        else {
            // [보간 이동]
            // Frame A와 Frame B 사이의 실제 이동 거리 계산
            // (서버가 보내준 두 점 사이의 거리가 얼마나 되는가?)
            float intervalDist = Vector3::Length(Vector3::Subtract(frameB->position, frameA->position));

            // 두 패킷 사이의 거리가 1cm 미만(0.01f)이면 '사실상 멈춤'으로 간주
            if (intervalDist < 0.01f) {
                // 멈춰 있을 때는 Frame의 상태를 최대한 존중하되, 명확히 멈췄으니 IDLE 처리
                AI_state = AI_STATE::MONSTER_IDLE;

               // if (frameA->AI_state == AI_STATE::MONSTER_PATROL) AI_state = frameA->AI_state;
               // if (frameB->AI_state == AI_STATE::MONSTER_PATROL) AI_state = frameB->AI_state;

                nextPos = position;
            }
            else {
                // 이동 중일 때는 무조건 서버가 알려준 FrameB(목표)의 상태를 그대로 따름!
                //AI_state = frameB->AI_state;
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