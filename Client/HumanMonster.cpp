#include "stdafx.h"
#include "HumanMonster.h"
#include "Player.h"
#include "AIComponent.h"
#include "Movement.h"
#include "SceneManager.h"
#include "MyPlayer.h"
#include "Animator.h"
#include "SoundManager.h"
#include "GameScene.h"
#include "State.h"

CHumanMonster::CHumanMonster()
    : CMonster(MON_TYPE::HUMAN_MONSTER)
{
    friction = 0.0f;
    SetFOV(120);
    respawn_time = 20.f;
}

CHumanMonster::~CHumanMonster()
{
}

void CHumanMonster::Update(float elapsedTime)
{
	CMonster::Update(elapsedTime);

	if (melee_knockback_timer > 0.0f) {
		float ratio = melee_knockback_timer / MELEE_KNOCKBACK_DURATION;
		velocity.x = melee_knockback_vel.x * ratio;
		velocity.z = melee_knockback_vel.z * ratio;

		melee_knockback_timer -= elapsedTime;

		if (melee_knockback_timer < 0.0f) 
            melee_knockback_timer = 0.0f;
	}
}

void CHumanMonster::OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers)
{
    CObject::OnCollect(renderers);

    auto animator = GetComponent<CAnimatorComponent>();
    if (animator) {
        animator->RenderSocketModel(CAnimatorComponent::HAND_R, NULL, "flapper");
    }
}

void CHumanMonster::OnIdleMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f) 
        return;

     // 시야 범위에 플레이어가 들어오는지 체크
    auto target = FindNearestPlayer();

    if (target) {

        // 타겟을 향하는 방향 벡터 및 평면(XZ) 거리 계산
        XMFLOAT3 dirVec = Vector3::Subtract(target->position, position);
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

        // 플레이어가 초기 위치로 복귀하는 동안에는 걷는 애니메이션이 나와야한다.
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
    if (melee_knockback_timer > 0.0f) 
        return;

    // (순찰 상태)
    // 타겟 탐색 (TRACE 전환)
    auto target = FindNearestPlayer();
    if (target) {

        // 타겟을 향하는 벡터 및 평면(XZ) 거리 계산
        XMFLOAT3 dirVec = Vector3::Subtract(target->position, position);
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
    if (melee_knockback_timer > 0.0f) 
        return;

    auto AIComponent = GetComponent<CAIComponent>();

    auto targetPlayer = target_player.lock();

    if (!targetPlayer) {
        // 타겟이 사라졌으면 IDLE로 복귀
        AIComponent->ChangeState(AI_STATE::MONSTER_IDLE);
        return;
    }

    // 타겟과의 방향 및 평면 거리 계산
    XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->position, position);
    dirVec.y = 0.0f; // Y축(높이) 차이는 무시하고 XZ 평면에서의 거리만 계산
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
    int ex = (int)roundf(targetPlayer->position.x / TILE_SIZE);
    int ez = (int)roundf(targetPlayer->position.z / TILE_SIZE);

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
    if (melee_knockback_timer > 0.0f) 
        return;

    velocity.x = 0.0f;
    velocity.z = 0.0f;

    attack_timer += elapsedTime;

    if (!hit_damage_dealt && attack_timer >= 0.45f) {

        CSoundManager::GetInstance().Play(SOUND_ID::jab);

        hit_damage_dealt = true;

        auto targetPlayer = target_player.lock();
        if (targetPlayer && targetPlayer->GetIsMyPlayer()) {
            XMFLOAT3 dirVec = Vector3::Subtract(targetPlayer->position, position);
            dirVec.y = 0.0f;

            XMFLOAT3 fwd       = Vector3::Normalize(XMFLOAT3{ look.x,  0.0f, look.z  });
            XMFLOAT3 right_vec = Vector3::Normalize(XMFLOAT3{ right.x, 0.0f, right.z });

            float forwardDist = Vector3::DotProduct(dirVec, fwd);
            float sideDist    = Vector3::DotProduct(dirVec, right_vec);

            constexpr float depth = 1.3f;  // 조절 가능
            constexpr float width = 0.8f;  // 조절 가능

            if (forwardDist >= -0.3f && forwardDist <= depth && fabsf(sideDist) <= width) {
                CSoundManager::GetInstance().Play(SOUND_ID::damaged1);
                uint32 hp = targetPlayer->GetHp();
                targetPlayer->SetHp(hp > 10 ? hp - 10 : 0);
                XMFLOAT3 knockbackDir = Vector3::Subtract(targetPlayer->position, position);
                static_cast<CMyPlayer*>(targetPlayer.get())->ApplyKnockback(knockbackDir, 0.6f, 0.f);
            }
        }
    }

    if (attack_timer >= 1.5f) {
        auto AIComponent = GetComponent<CAIComponent>();
        if (AIComponent)
            AIComponent->ChangeState(AI_STATE::MONSTER_TRACE);
    }
}

void CHumanMonster::ApplyMeleeHit(const XMFLOAT3& fromPos)
{
    CMonster::ApplyMeleeHit(fromPos);

    melee_hit_count++;
    if (melee_hit_count >= MAX_MELEE_HITS) {
        auto* ai = GetComponent<CAIComponent>();
        if (ai) {
            auto cur = ai->GetCurrentState();
            if (!cur || cur->GetType() != AI_STATE::MONSTER_FLEE)
                ai->ChangeState(AI_STATE::MONSTER_FLEE);
        }
    }
}

void CHumanMonster::OnFleeEnter()
{
    CSoundManager::GetInstance().Play(SOUND_ID::girl_flee);

    flee_timer = FLEE_DURATION;
    nav_path.clear();
    path_refresh_timer = 0.0f;
    velocity.x = 0.0f;
    velocity.z = 0.0f;

    auto targetPlayer = target_player.lock();
    if (!targetPlayer) {
        auto nearest = FindNearestPlayer();
        if (nearest)
            SetTarget(nearest);
        targetPlayer = target_player.lock();
    }

    if (targetPlayer) {
        XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->position);
        awayDir.y = 0.0f;
        float len = Vector3::Length(awayDir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }

    auto scene = CSceneManager::GetInstance().GetActiveScene();
    if (auto gameScene = dynamic_cast<CGameScene*>(scene)) {
        XMFLOAT3 itemSpawnPos = position;
        itemSpawnPos.y = 0.2f;
        gameScene->SpawnWorldItem(20, itemSpawnPos);
    }
}

void CHumanMonster::OnFleeMove(float elapsedTime)
{
    if (melee_knockback_timer > 0.0f) return;

    flee_timer -= elapsedTime;
    if (flee_timer <= 0.0f) {
        MarkForDelete();
        return;
    }

    auto targetPlayer = target_player.lock();
    if (targetPlayer) {
        XMFLOAT3 awayDir = Vector3::Subtract(position, targetPlayer->position);
        awayDir.y = 0.0f;
        float len = Vector3::Length(awayDir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(awayDir.x, awayDir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }

    velocity.x = look.x * FLEE_SPEED;
    velocity.z = look.z * FLEE_SPEED;
}

void CHumanMonster::OnFleeExit()
{
    flee_timer = 0.0f;
    velocity.x = 0.0f;
    velocity.z = 0.0f;
}

void CHumanMonster::OnIdleEnter()
{
    ResetIdleTimer();
    nav_path.clear();
    path_refresh_timer = 0.0f;
}

void CHumanMonster::OnPatrolEnter()
{
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
        XMFLOAT3 dir = Vector3::Subtract(targetPlayer->position, position);
        dir.y = 0.0f;
        float len = Vector3::Length(dir);
        if (len > 0.001f) {
            float yaw = XMConvertToDegrees(atan2f(dir.x, dir.z));
            SetYaw(yaw);
            SetYawPitch(yaw, 0.0f);
        }
    }
}