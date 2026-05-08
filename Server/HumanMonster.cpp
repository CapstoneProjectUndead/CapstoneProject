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

        const float TILE_SIZE = 2.0f;
        path_refresh_timer += elapsedTime;
        if (path_refresh_timer >= 0.3f || nav_path.empty()) {
            path_refresh_timer = 0.0f;
            int sx = (int)roundf(position.x / TILE_SIZE);
            int sz = (int)roundf(position.z / TILE_SIZE);
            int ex = (int)roundf(origin_position.x / TILE_SIZE);
            int ez = (int)roundf(origin_position.z / TILE_SIZE);
            nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
        }

        XMFLOAT3 moveDir = dirToOrigin;
        if (!nav_path.empty()) {
            XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
            XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
            toWp.y = 0.0f;
            if (Vector3::Length(toWp) > 0.1f)
                moveDir = toWp;
        }

        float returnYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
        SetYaw(returnYaw);
        SetYawPitch(returnYaw, 0.0f);

        float walk_speed = 0.5f;
        velocity.x = look.x * walk_speed;
        velocity.z = look.z * walk_speed;

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

    const float TILE_SIZE = 2.0f;
    int sx = (int)roundf(position.x / TILE_SIZE);
    int sz = (int)roundf(position.z / TILE_SIZE);
    XMFLOAT3 targetPos = targetPlayer->GetPosition();
    int ex = (int)roundf(targetPos.x / TILE_SIZE);
    int ez = (int)roundf(targetPos.z / TILE_SIZE);

    // Bresenham 직선으로 벽 여부 확인 — 막힘 없으면 직진, 막히면 BFS
    bool hasWall = false;
    {
        int x = sx, z = sz;
        int dx = abs(ex - sx), dz = abs(ez - sz);
        int stepX = (ex > sx) ? 1 : -1;
        int stepZ = (ez > sz) ? 1 : -1;
        int err = dx - dz;

        while (x != ex || z != ez) {
            if (MapGenerator::IsBlockedStructure(x, z)) {
                hasWall = true;
                break;
            }
            int e2 = 2 * err;
            if (e2 > -dz) { err -= dz; x += stepX; }
            if (e2 <  dx) { err += dx; z += stepZ; }
        }
    }

    XMFLOAT3 moveDir = dirVec;
    if (hasWall) {
        path_refresh_timer += elapsedTime;
        if (path_refresh_timer >= 0.2f || nav_path.empty()) {
            path_refresh_timer = 0.0f;
            nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
        }
        if (!nav_path.empty()) {
            XMFLOAT3 wpWorld = { nav_path[0].x * TILE_SIZE, position.y, nav_path[0].y * TILE_SIZE };
            XMFLOAT3 toWp = Vector3::Subtract(wpWorld, position);
            toWp.y = 0.0f;
            if (Vector3::Length(toWp) > 0.1f)
                moveDir = toWp;
        }
    }
    else {
        nav_path.clear();
        path_refresh_timer = 0.0f;
    }

    float targetYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
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

    if (!hit_damage_dealt && attack_timer >= 0.45f) {
        hit_damage_dealt = true;

        auto targetPlayer = target_player.lock();
        if (targetPlayer) {

            S_PlaySound soundPkt;
            soundPkt.is_global = false;
            soundPkt.scene_type = current_scene_type;
            soundPkt.sound_id = SOUND_ID::jab;

            XMFLOAT3 monsterPos = GetPosition();
            soundPkt.x = monsterPos.x;
            soundPkt.y = monsterPos.y;
            soundPkt.z = monsterPos.z;

            auto sendBuffer = MAKE_SEND_BUFFER(soundPkt);
            if (auto scene = GetScene()) {
                scene->BroadCast(sendBuffer);
            }

            XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->GetPosition(), position);
            dirVec.y = 0.0f;

            XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
            XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

            float forwardDist = Vector3::DotProduct(dirVec, fwd);
            float sideDist    = Vector3::DotProduct(dirVec, right_vec);

            constexpr float depth = 1.3f;
            constexpr float width = 0.8f;

            if (forwardDist >= -0.3f && forwardDist <= depth && fabsf(sideDist) <= width) {

                soundPkt.is_global = false;
                soundPkt.scene_type = current_scene_type;
                soundPkt.sound_id = SOUND_ID::damaged1;

                XMFLOAT3 targetPos = targetPlayer->GetPosition();
                soundPkt.x = targetPos.x;
                soundPkt.y = targetPos.y;
                soundPkt.z = targetPos.z;

                sendBuffer = MAKE_SEND_BUFFER(soundPkt);
                if (auto scene = GetScene()) {
                    scene->BroadCast(sendBuffer);
                }

                uint32 hp = targetPlayer->GetHp();
                targetPlayer->SetHp(hp > 10 ? hp - 10 : 0);
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
    nav_path.clear();
    path_refresh_timer = 0.0f;
}

void CHumanMonster::OnPatrolEnter()
{
    // 순찰 타이머 리셋
    ResetPatrolTimers();
}

void CHumanMonster::OnTraceEnter()
{
    nav_path.clear();
    path_refresh_timer = 0.0f;
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