#include "pch.h"
// Server쪽 HumanMonster
#include "HumanMonster.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "Room.h"
#include "Scene.h"

CHumanMonster::CHumanMonster()
    : CMonster(MON_TYPE::HUMAN_MONSTER)
{
    friction = 0.0f;
    SetFOV(120);
}

CHumanMonster::~CHumanMonster()
{
}

void CHumanMonster::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);
}

void CHumanMonster::OnIdleMove(float elapsedTime)
{
    // (Idle 상태)

    // 시야 범위에 플레이어가 들어오는지 체크
    auto target = FindNearestPlayer();

    if (target) {

        // 타겟을 향하는 방향 벡터 및 평면(XZ) 거리 계산
        XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
        dirVec.y = 0.0f; // Y축(높이) 차이는 무시
        float dist = Vector3::Length(dirVec);

        // 인식 거리(recog_range) 내에 있는지 1차 확인
        if (dist <= recog_range && dist > 0.001f) {

            // 타겟 방향 벡터 정규화 (길이를 1로 만듦)
            XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };

            // 내적(Dot Product) 계산 (Object의 look 벡터 활용)
            float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

            // 플레이어가 시야각 안에 들어왔다면 추적 시작
            if (dotProduct >= cos_threshold) {
                SetTarget(target);

                auto AIComponent = GetComponent<CAIComponent>();
                if (AIComponent) {
                    AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
                    return;
                }
            }
        }
    }

    // 초기 자리(origin_position)로 복귀하기
    XMFLOAT3 dirToOrigin = Vector3::Subtract(origin_position, position);
    dirToOrigin.y = 0.0f;
    float distToOrigin = Vector3::Length(dirToOrigin);

    if (distToOrigin > 0.1f) {

        // 몬스터가 초기 위치로 복귀하는 동안에는 걷는 애니메이션이 나와야한다.
        AI_state = AI_STATE::MONSTER_PATROL;

        // 초기 자리를 향해 방향 틀기
        float returnYaw = XMConvertToDegrees(atan2f(dirToOrigin.x, dirToOrigin.z));
        SetYaw(returnYaw);
        SetYawPitch(returnYaw, 0.0f);

        // 초기 자리를 향해 걷기 (산책 속도)
        float walk_speed = 0.5f;
        velocity.x = look.x * walk_speed;
        velocity.z = look.z * walk_speed;

        // 아직 도착하지 않았으므로 idle_timer는 증가시키지 않고 여기서 함수 종료!
        return;
    }

    // 초기 지점으로 복귀하면 
    {
        SetYaw(0.0f);
        SetYawPitch(0.0f, 0.0f);
        AI_state = AI_STATE::MONSTER_IDLE;
    }

    // 속도 0 고정 (휴식)
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 타이머 체크: 5초 쉬었으면 순찰(PATROL)하러 출발
    idle_timer += elapsedTime;

    if (idle_timer >= 5.0f) {

        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_PATROL);
    }
}

void CHumanMonster::OnPatrolMove(float elapsedTime)
{
    // (순찰 상태)
    // 타겟 탐색 (TRACE 전환)
    auto target = FindNearestPlayer();
    if (target) {

        // 타겟을 향하는 벡터 및 평면(XZ) 거리 계산
        XMFLOAT3 dirVec = Vector3::Subtract(target->GetPosition(), position);
        dirVec.y = 0.0f; // 높이 차이는 무시 (위아래는 안 보고 평면 시야만 체크)
        float dist = Vector3::Length(dirVec);

        // 일단 인식 거리(recog_range) 안에 들어왔는지 1차 확인
        if (dist <= recog_range && dist > 0.001f) {

            // 타겟 방향 벡터 정규화 (길이를 1로 만듦)
            XMFLOAT3 dirNorm = { dirVec.x / dist, 0.0f, dirVec.z / dist };

            // 내적(Dot Product) 계산 (Object의 look 벡터 활용)
            // (내적 = x끼리 곱 + z끼리 곱)
            float dotProduct = (look.x * dirNorm.x) + (look.z * dirNorm.z);

            // 내적값이 임계값 이상이면 시야각 안에 있는 것!
            if (dotProduct >= cos_threshold) {

                // 발각!
                SetTarget(target);
                auto AIComponent = GetComponent<CAIComponent>();
                if (AIComponent) {
                    AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
                    return;
                }
            }
        }
    }

    patrol_timer += elapsedTime;
    turn_timer += elapsedTime;

    // 전체 배회 시간 (5초)
    if (patrol_timer >= 5.0f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 방향 전환 (2초)
    if (turn_timer >= 2.0f) {
        float newYaw = yaw + 180.0f;
        SetYaw(newYaw);
        SetYawPitch(newYaw, 0.0f);
        turn_timer = 0.0f;
    }

    // 이동 처리
    float walk_speed = 0.4f;
    velocity.x = look.x * walk_speed;
    velocity.z = look.z * walk_speed;
}

void CHumanMonster::OnTraceMove(float elapsedTime)
{
    auto AIComponent = GetComponent<CAIComponent>();

    if (!target_player) {
        // 타겟이 사라졌으면 IDLE로 복귀
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 타겟과의 방향 및 평면 거리 계산
    XMFLOAT3 dirVec = Vector3::Subtract(target_player->GetPosition(), position);
    dirVec.y = 0.0f; // Y축(높이) 차이는 무시하고 XZ 평면에서의 거리만 계산
    float dist = Vector3::Length(dirVec);

    // 상태 전환 (State Transition) 판단
    // 조건 A: 타겟이 인식 범위 밖으로 도망갔을 때 -> 추적 포기
    if (dist > recog_range) {
        target_player = nullptr; // 타겟 초기화
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }
    // 조건 B: 타겟이 공격 범위(0.2f) 안으로 들어왔을 때 -> 공격 시작!
    else if (dist <= 0.2f) {
        AIComponent->ChangeState(AI_STATE::MONSTER_ATTACK);
        return;
    }

    // 타겟을 향해 회전 (Rotation)
    // atan2f 함수를 이용해 목표 방향 벡터를 각도(Yaw)로 변환
    float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));

    SetYaw(targetYaw);             // 물리적인 앞 방향(look) 갱신
    SetYawPitch(targetYaw, 0.0f);  // 그래픽(모델링) 방향 갱신

    // 타겟을 향해 돌진 (Movement)
    // 마찰력이 0인 상태이므로, IDLE(0.4f)보다 훨씬 빠른 속도로 직접 꽂아줍니다.
    velocity.x = look.x * trace_speed;
    velocity.z = look.z * trace_speed;
}

void CHumanMonster::OnAttackMove(float elapsedTime)
{
    // 공격 중에는 미끄러지지 않게 이동 속도 0으로 고정
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 타이머 증가
    attack_timer += elapsedTime;

    // 공격 애니메이션 길이 or 쿨타임이 지나면?
    if (attack_timer >= 1.5f) {

        // 실제 데미지 판정 로직은 서버의 이 시점(또는 타이머 중간)에 수행!
        // 예: target_player->TakeDamage(10);

        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent) {
            // 공격이 끝났으니 다시 거리를 재기 위해 TRACE 상태로 전환
            // (TRACE 상태에서 거리가 가까우면 다음 프레임에 다시 ATTACK으로 돌아옴)
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
        }
    }
}

void CHumanMonster::OnIdleEnter()
{
    // IDLE 상태로 진입하면 10초 동안 쉬는데
    // 그 타이머를 0으로 만든다.
    ResetIdleTimer();
}

void CHumanMonster::OnPatrolEnter()
{
    // 순찰 타이머 리셋
    ResetPatrolTimers();
}

void CHumanMonster::OnAttackEnter()
{
    ResetAttackTimer();

    // 1. 공격 상태 진입 시, 어떤 공격을 할지 결정 (발 구르기 vs 파리채)
    // 예: int pattern = rand() % 2; 
    // DecideAttackPattern(); 

    // 2. 이동 속도를 0으로 만들어서 공격 중에 미끄러지지 않게 고정
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    // 3. 공격 타이머 초기화 (이제 CHumanMonster의 멤버 변수이므로 직접 접근 가능!)
    attack_timer = 0.0f;
}