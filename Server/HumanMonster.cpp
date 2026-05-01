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
    auto targetPlayer = target_player.lock();

    if (!targetPlayer) {
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->GetPosition(), position);
    dirVec.y = 0.0f;
    float dist = Vector3::Length(dirVec);

    attack_cooldown_timer += elapsedTime;

    if (dist > recog_range) {
        target_player.reset();
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }
    else if (dist <= attack_range && attack_cooldown_timer >= 0.4f) {
        AIComponent->ChangeState(AI_STATE::MONSTER_ATTACK);
        return;
    }
    else if (dist <= attack_range) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        AI_state = AI_STATE::MONSTER_IDLE;
        return;
    }

    AI_state = AI_STATE::MONSTER_TRACE;

    float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
    SetYaw(targetYaw);
    SetYawPitch(targetYaw, 0.0f);

    velocity.x = look.x * trace_speed;
    velocity.z = look.z * trace_speed;
}

void CHumanMonster::OnAttackMove(float elapsedTime)
{
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    attack_timer += elapsedTime;

    if (!hit_damage_dealt && attack_timer >= 0.7f) {
        hit_damage_dealt = true;

        auto targetPlayer = target_player.lock();
        if (targetPlayer) {
            XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->GetPosition(), position);
            dirVec.y = 0.0f;

            XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
            XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

            float forwardDist = Vector3::DotProduct(dirVec, fwd);
            float sideDist    = Vector3::DotProduct(dirVec, right_vec);

            constexpr float depth = 1.5f;
            constexpr float width = 1.0f;

            if (forwardDist >= -0.3f && forwardDist <= depth && fabsf(sideDist) <= width) {
                uint32 hp = targetPlayer->GetHp();
                targetPlayer->SetHp(hp > 15 ? hp - 15 : 0);
                XMFLOAT3 knockbackDir = Vector3::Subtract(targetPlayer->GetPosition(), position);
                targetPlayer->ApplyKnockback(knockbackDir, 0.6f, 0.f);
            }
        }
    }

    if (attack_timer >= 1.5f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
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

    velocity.x            = 0.0f;
    velocity.z            = 0.0f;
    attack_timer          = 0.0f;
    hit_damage_dealt      = false;
    attack_cooldown_timer = 0.0f;

    auto targetPlayer = target_player.lock();
    if (targetPlayer) {
        XMFLOAT3 dir = Vector3::Subtract(targetPlayer->GetPosition(), position);
        dir.y = 0.0f;
        float len = Vector3::Length(dir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }
}