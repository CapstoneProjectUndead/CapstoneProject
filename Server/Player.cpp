#include "pch.h"
// Server쪽 Player
#include "Player.h"
#include "Collider.h"
#include "Movement.h"
#include "Scene.h"
#include "GameScene.h"
#include "MineableObject.h"
#include "Room.h"
#include "PhysicsManager.h"
#include "Item.h"
#include "Inventory.h"
#include "Ghost.h"
#include "HumanMonster.h"


CPlayer::CPlayer()
	: CObject(OBJECT_TYPE::PLAYER)
    , last_processed_seq(0)
    , ping(0.0f)
    , dt_ping_accumulator(0.0f)
    , is_guest(false)
	, state(PLAYER_STATE::IDLE)
    , equipped_item_id(0)
    , equipped_item_sub_type(ITEM_SUB_TYPE::NONE)
    , is_ready(false)
    , is_dowsing(false)
    , accumulate_stamina(100.f)
    , stamina_exhausted(false)
    , dig_timer(0.f)
    , knockback_vel{}
    , knockback_timer(0.0f)
    , is_stunned(false)
    , stun_timer(0.0f)
    , is_possessed(false)
    , possession_timer(0.0f)
    , c_hold_timer(0.0f)
    , last_c_input(false)
    , start_jump(false)
    , possessed_path_refresh_timer(0.0f)
    , possessed_wander_target{}
    , possessed_is_waiting(false)
    , possessed_wait_timer(0.0f)
    , possession_contact_timer(0.0f)
    , possessed_spray_hit_count(0)
    , bare_hand_dig_timer(0.0f)
    , gold(0)
{

}

CPlayer::~CPlayer()
{
    if (auto r = room.lock()) {
        if (auto physicsManager = r->GetScenes()[(UINT)current_scene_type]->GetPhysicsManager()) {
            physicsManager->EraseCollider(GetComponent<CColliderComponent>());
        }
    }
}

void CPlayer::Update(const float elapsedTime)
{
    last_simulated_time = static_cast<float>(g_server_total_time);

    SetYawPitch(yaw, pitch);
    UpdateWorldMatrix();

    if (is_possessed)
        UpdatePossession(elapsedTime);

    ProcessInputQueue(elapsedTime);
}

void CPlayer::ProcessInputQueue(const float elapsedTime) // elapsedTime == g_targetDT(16.6ms)
{
    if (is_possessed) {
        input_queue.clear();
        InputData emptyInput{ false, false, false, false, false, false };
        SimulateMove(emptyInput, elapsedTime, false);
        return;
    }

    if (!input_queue.empty())
    {
        // 쌓인 패킷이 있다면, 각 패킷마다 시뮬레이션을 돌림
        while (!input_queue.empty())
        {
            PendingInput pending = input_queue.front();
            input_queue.pop_front();

            // (가속 -> 속도제한 -> 이동 -> 감속)
            last_c_input = pending.input.c;
            SimulateMove(pending.input, elapsedTime);

            // 서버가 해당 시퀀스넘버의 클라 입력을 처리했다.
            last_processed_seq = pending.seq_num;

            // 장부 기록 
            ServerFrameHistory frame{};
            frame.input = pending.input;
            frame.seq_num = last_processed_seq;
            frame.position = position;
            frame.state = state;
            frame.timestamp = static_cast<float>(last_simulated_time);

            RecordServerFrameHistory(frame);

            last_simulated_time += g_targetDT;
        }
    }
    else
    {
        // 입력이 없어도 마찰/중력 계산을 위해 1회 업데이트 (state는 변경하지 않음)
        InputData emptyInput{ false, false, false, false, false, false, false };
        emptyInput.c = last_c_input;
        SimulateMove(emptyInput, elapsedTime, false);

        if (last_simulated_time < g_server_total_time)
            last_simulated_time = static_cast<float>(g_server_total_time);

        // 장부 기록 
        ServerFrameHistory frame{};
        frame.input = emptyInput;
        frame.seq_num = last_processed_seq;
        frame.position = position;
        frame.state = state;
        frame.timestamp = static_cast<float>(last_simulated_time);

        RecordServerFrameHistory(frame);
    }
}

void CPlayer::SimulateMove(const InputData& input, float elapsedTime, bool updateState)
{
    if (is_possessed) {
        if (knockback_timer > 0.0f) {
            float ratio = knockback_timer / 0.3f;
            velocity.x = knockback_vel.x * ratio;
            velocity.z = knockback_vel.z * ratio;
            knockback_timer -= elapsedTime;
            if (knockback_timer < 0.0f)
                knockback_timer = 0.0f;
        }
        else if (stun_timer > 0.0f) {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            stun_timer -= elapsedTime;
            if (stun_timer < 0.0f) {
                is_stunned = false;
                stun_timer = 0.0f;
            }
        }
        CObject::Update(elapsedTime);
        return;
    }

    // C키 홀드 → 인접 빙의 플레이어 해제 OR 빈사 플레이어 소생
    UpdateCHoldAction(input, elapsedTime);

    // --------------------
    // 입력 처리 및 방향 계산
    // --------------------
    XMFLOAT3 dir{ 0.f, 0.f, 0.f };
    if (input.w) dir.z++;
    if (input.s) dir.z--;
    if (input.a) dir.x--;
    if (input.d) dir.x++;

    // 점프 (빈사/사망 시 차단)
    if (state == PLAYER_STATE::ALMOST_DEAD || state == PLAYER_STATE::DEAD) {
        start_jump = false;
    }
    else {
        start_jump = (input.space && is_grounded && !stamina_exhausted);
        if (input.space && !stamina_exhausted) {
            if (auto move = GetComponent<CMovementComponent>())
                move->Jump();
        }
    }

    // 상태 갱신
    bool isMoving = (dir.x != 0 || dir.z != 0);

    // 움찔거리는 거 방지 (Client PredictMove와 동일 로직)
    grounded_timer = is_grounded ? 0.1f : (grounded_timer - elapsedTime);

    if (updateState && state != PLAYER_STATE::ALMOST_DEAD && state != PLAYER_STATE::DEAD) {

        bool isBareHand = (equipped_item_id == 0);
        bool isShovel = equipped_item_id >= 1 && equipped_item_id <= 4;

        if (equipped_item_sub_type == ITEM_SUB_TYPE::TOOL
            && !isShovel
            && current_scene_type == SCENE_TYPE::GAME) {
            ProcessVisibleObjectMining(input, elapsedTime, isMoving);
        }
        else if (current_scene_type == SCENE_TYPE::GAME
            && (isBareHand || isShovel)) {
            ProcessUnVisibleObjectMining(input, elapsedTime, isMoving);
        }

        if (current_scene_type == SCENE_TYPE::GAME) {
            switch (equipped_item_sub_type) 
            {
                case ITEM_SUB_TYPE::MELEE_WEAPON:
                {
                    ProcessMeleeAttack(input, elapsedTime);
                }
                break;
                case ITEM_SUB_TYPE::RANGED_WEAPON:
                {
                    ProcessRangedAttack(input, elapsedTime);
                }
                break;
            }
        }

        if (grounded_timer > 0.0f) {

            if (!isMoving) {

                if (state != PLAYER_STATE::DIG && state != PLAYER_STATE::ATTACK)
                    state = PLAYER_STATE::IDLE;

                if (is_grounded && knockback_timer <= 0.0f) {
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                }
            }
            else {
                // 이동 입력이 들어오면 채굴 취소
                dig_timer = 0.0f;
                dig_sound_timer = -1.0f;
                bare_hand_dig_timer = 0.0f;

                if (auto move = GetComponent<CMovementComponent>()) {
                    if (input.shift && !stamina_exhausted && !is_dowsing) {
                        if (state != PLAYER_STATE::ATTACK)
                            state = PLAYER_STATE::RUN;
                        move->Run();
                    }
                    else {
                        if (state != PLAYER_STATE::ATTACK)
                            state = PLAYER_STATE::WALK;
                        move->UnRun();
                    }
                }
            }
        }
        else {
            dig_timer = 0.0f;
            dig_sound_timer = -1.0f;
            state = PLAYER_STATE::JUMP; // 확실히 공중일 때만 점프 상태
        }
    }
    else {
        // updateState=false: 물리만 계산, velocity 마찰 적용 (입력 없는 틱)
        if (is_grounded && !isMoving && knockback_timer <= 0.0f) {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
        }
    }

    UpdateStamina(elapsedTime);

    if (knockback_timer <= 0.0f && stun_timer <= 0.0f
        && state != PLAYER_STATE::ALMOST_DEAD && state != PLAYER_STATE::DEAD
        && (dir.x != 0 || dir.z != 0)) {
        if (auto move = GetComponent<CMovementComponent>())
            move->Move(dir, elapsedTime);
    }

    if (knockback_timer > 0.0f) {

        float ratio = knockback_timer / 0.3f;
        velocity.x += knockback_vel.x * ratio;
        velocity.z += knockback_vel.z * ratio;

        knockback_timer -= elapsedTime;

        if (knockback_timer < 0.0f)
            knockback_timer = 0.0f;
    }

    if (stun_timer > 0.0f) {
        stun_timer -= elapsedTime;
        if (stun_timer < 0.0f) {
            is_stunned = false;
            stun_timer = 0.0f;
        }
    }

    velocity.x += separation_push.x;
    velocity.z += separation_push.z;
    separation_push = {};

    // Movement와 Collider Update
    CObject::Update(elapsedTime);

    // (5월 17일 추가)separation push로 인한 velocity가 broadcast에 누출되지 않도록
    if (state == PLAYER_STATE::IDLE) {
        velocity.x = 0.f;
        velocity.z = 0.f;
    }
}

void CPlayer::ApplySeparation(const map<uint64, shared_ptr<CPlayer>>& allPlayers, float scale)
{
	constexpr float SEPARATION_RADIUS = 0.5f;
	constexpr float SEPARATION_FORCE  = 4.0f;

	XMFLOAT3 push = { 0.f, 0.f, 0.f };
	for (const auto& [id, other] : allPlayers) {
		if (other.get() == this)
			continue;

		// 죽은 플레이어(시체)는 분리 대상에서 제외 (살아있는 plr가 시체에 밀려나지 않게)
		if (other->GetState() == PLAYER_STATE::DEAD)
			continue;

		XMFLOAT3 diff = Vector3::Subtract(position, other->GetPosition());
		diff.y = 0.0f;
		float dist = Vector3::Length(diff);

		if (dist < 0.01f) {
			// 완전히 겹쳐있을 때: ID 대소로 방향 결정하여 강제 분리
			float sign = (GetID() > other->GetID()) ? 1.0f : -1.0f;
			push.x += sign * SEPARATION_FORCE;
		}
		else if (dist < SEPARATION_RADIUS) {
			float invDist  = 1.0f / dist;
			float strength = (SEPARATION_RADIUS - dist) / SEPARATION_RADIUS;
			push.x += diff.x * invDist * strength * SEPARATION_FORCE;
			push.z += diff.z * invDist * strength * SEPARATION_FORCE;
		}
	}
	separation_push.x = push.x * scale;
	separation_push.z = push.z * scale;
}

void CPlayer::ApplyKnockback(XMFLOAT3 dir, float force, float stun_duration)
{
    dir.y = 0.0f;
    float len = Vector3::Length(dir);

    if (len < 0.001f)
        return;

    knockback_vel   = { dir.x / len * force, 0.0f, dir.z / len * force };
    knockback_timer = 0.3f;
    if (stun_duration > 0.0f)
        ApplyStun(stun_duration);
}

void CPlayer::ApplyStun(float time)
{
    is_stunned = true;
    stun_timer = time;
}

void CPlayer::ApplyPossession()
{
    if (is_returned || IsIncapacitated())
        return;

    is_possessed    = true;
    possession_timer = 30.0f;

    accumulate_stamina = 0.0f;
    stamina_exhausted  = true;
    stat.stamina       = 0;

    possessed_nav_path.clear();
    possessed_path_refresh_timer  = 0.0f;
    possessed_wander_target       = GetRandomPossessedTarget();
    possessed_is_waiting          = false;
    possessed_wait_timer          = 0.0f;
    possession_contact_timer      = 0.0f;
    possessed_spray_hit_count     = 0;
}

void CPlayer::UpdatePossession(float elapsedTime)
{
    possession_timer        -= elapsedTime;
    possession_contact_timer += elapsedTime;

    if (possession_timer <= 0.0f) {
        // 빙의 해제 Sound 패킷 전송
        SendSoundPacket(false, SOUND_ID::devil_laugh1, GetPosition());

        possession_timer = 0.0f;
        is_possessed     = false;
        velocity.x       = 0.0f;
        velocity.z       = 0.0f;
        state            = PLAYER_STATE::RUN;
        return;
    }

    constexpr float TILE_SIZE          = 2.0f;
    constexpr float ARRIVE_DIST        = 0.4f;
    constexpr float POSSESS_SPEED      = 4.0f;
    constexpr float ATTACK_INTERVAL    = 1.5f; // 공격 후 대기 시간 = 데미지 쿨다운

    auto nearOther = FindNearestOtherPlayer();

    if (nearOther) {

        XMFLOAT3 dirVec = Vector3::Subtract(nearOther->GetPosition(), position);
        dirVec.y = 0.0f;
        float dist = Vector3::Length(dirVec);

        if (dist > 0.001f) {
            float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
            SetYaw(targetYaw);
            SetYawPitch(targetYaw, 0.0f);
        }

        if (possession_contact_timer >= ATTACK_INTERVAL) {
            velocity.x = look.x * POSSESS_SPEED;
            velocity.z = look.z * POSSESS_SPEED;
            state      = PLAYER_STATE::RUN;
        }
        else {
            velocity.x = 0.0f;
            velocity.z = 0.0f;
            state      = PLAYER_STATE::IDLE;
        }

        auto* myCol    = GetComponent<CColliderComponent>();
        auto* otherCol = nearOther->GetComponent<CColliderComponent>();

        if (myCol && otherCol && myCol->Intersects(otherCol))
        {
            if (possession_contact_timer >= ATTACK_INTERVAL) {

                SendSoundPacket(false, SOUND_ID::damaged1, GetPosition());

                uint32 hp = nearOther->GetHp();
                nearOther->SetHp(hp > 10 ? hp - 10 : 0);

                uint32 myHp = GetHp();
                SetHp(myHp > 10 ? myHp - 10 : 0);

                possession_contact_timer = 0.0f;

                XMFLOAT3 knockbackDir = Vector3::Subtract(nearOther->GetPosition(), position);
                nearOther->ApplyKnockback(knockbackDir, 0.5f, 0.0f);
            }
        }

        return;
    }

    // 타 플레이어 없음 → BFS 랜덤 배회
    if (possessed_is_waiting) {

        velocity.x = 0.0f;
        velocity.z = 0.0f;

        possessed_wait_timer -= elapsedTime;

        if (possessed_wait_timer <= 0.0f) {
            possessed_wander_target      = GetRandomPossessedTarget();
            possessed_nav_path.clear();
            possessed_path_refresh_timer = 0.0f;
            possessed_is_waiting         = false;
        }

        return;
    }

    XMFLOAT3 dirVec = Vector3::Subtract(possessed_wander_target, position);
    dirVec.y = 0.0f;
    float dist = Vector3::Length(dirVec);

    if (dist < ARRIVE_DIST) {

        velocity.x           = 0.0f;
        velocity.z           = 0.0f;
        possessed_is_waiting = true;
        possessed_wait_timer = 0.3f + (rand() % 5) * 0.1f;

        return;
    }

    possessed_path_refresh_timer += elapsedTime;

    if (possessed_path_refresh_timer >= 0.2f || possessed_nav_path.empty()) {

        possessed_path_refresh_timer = 0.0f;
        int sx = (int)roundf(position.x / TILE_SIZE);
        int sz = (int)roundf(position.z / TILE_SIZE);
        int ex = (int)roundf(possessed_wander_target.x / TILE_SIZE);
        int ez = (int)roundf(possessed_wander_target.z / TILE_SIZE);
        possessed_nav_path = MapGenerator::FindPath(sx, sz, ex, ez);
    }

    XMFLOAT3 moveDir = dirVec;
    if (!possessed_nav_path.empty()) {

        XMFLOAT3 wpWorld = { possessed_nav_path[0].x * TILE_SIZE, position.y, possessed_nav_path[0].y * TILE_SIZE };
        XMFLOAT3 toWp    = Vector3::Subtract(wpWorld, position);
        toWp.y = 0.0f;

        if (Vector3::Length(toWp) > 0.1f)
            moveDir = toWp;
    }

    float moveYaw = XMConvertToDegrees(atan2f(moveDir.x, moveDir.z));
    SetYaw(moveYaw);
    SetYawPitch(moveYaw, 0.0f);

    velocity.x = look.x * POSSESS_SPEED;
    velocity.z = look.z * POSSESS_SPEED;
    state      = PLAYER_STATE::RUN;
}

void CPlayer::UpdateCHoldAction(const InputData& input, const float elapsedTime)
{
    if (current_scene_type != SCENE_TYPE::GAME)
        return;

    // 본인이 빙의/다우징/빈사 중이면 C-홀드 액션 불가
    if (is_possessed || is_dowsing || IsIncapacitated()) {
        c_hold_timer = 0.0f;
        c_hold_target_type = CHOLD_TARGET::NONE;
        return;
    }

    if (!input.c) {
        c_hold_timer = 0.0f;
        c_hold_target_type = CHOLD_TARGET::NONE;
        return;
    }

    // C키가 눌린 상태에서 매 프레임 범위 내 후보 탐색 (빙의 플레이어 OR 빈사 플레이어)
    shared_ptr<CPlayer> target = nullptr;
    CHOLD_TARGET targetType = CHOLD_TARGET::NONE;
    auto room = GetRoom();
    if (room) {
        CScene* scene = room->GetScenes()[(UINT)current_scene_type].get();
        if (scene) {
            for (auto& [id, player] : scene->GetPlayers()) {

                if (!player || id == obj_id)
                    continue;

                bool isPossessed = player->GetIsPossessed();
                bool isRescuable = (player->GetState() == PLAYER_STATE::ALMOST_DEAD);
                if (!isPossessed && !isRescuable)
                    continue;

                XMFLOAT3 diff = Vector3::Subtract(player->GetPosition(), position);
                diff.y = 0.0f;

                if (Vector3::Length(diff) <= 0.9f) {
                    target = player;
                    targetType = isPossessed ? CHOLD_TARGET::POSSESSION : CHOLD_TARGET::RESCUE;
                    break;
                }
            }
        }
    }

    if (target) {
        // 대상 종류가 바뀌면 타이머 리셋 (예: 빙의 대상 → 빈사 대상으로 시선 이동)
        if (c_hold_target_type != targetType) {
            c_hold_timer = 0.0f;
            c_hold_target_type = targetType;
        }

        c_hold_timer += elapsedTime;

        float threshold = (targetType == CHOLD_TARGET::RESCUE) ? 7.0f : 5.0f;
        if (c_hold_timer >= threshold) {
            if (targetType == CHOLD_TARGET::POSSESSION) {
                // 빙의 해제 Sound 패킷 전송
                SendSoundPacket(false, SOUND_ID::devil_laugh1, GetPosition());
                target->SetPossessed(false);
                target->SetPossessionTimer(0.0f);
            }
            else { // RESCUE
                // 빈사 소생: HP 50% 복원 + 상태 IDLE (클라가 다음 S_Move_Player에서 카메라 1인칭 자동 복귀)
                target->SetHp(target->GetMaxHp() / 2);
                target->SetState(PLAYER_STATE::IDLE);
            }

            c_hold_timer = 0.0f;
            c_hold_target_type = CHOLD_TARGET::NONE;
        }
    }
    else {
        // 클라이언트가 C키를 누르고 있었지만 대상이 범위를 벗어났으면 실패 패킷 전송 (클라 진행바 리셋)
        if (c_hold_timer > 0.0f) {
            S_CHoldFail failPkt;
            failPkt.player_id = GetID();
            auto sendBuffer = MAKE_SEND_BUFFER(failPkt);
            if (auto s = session.lock())
                s->DoSend(sendBuffer);
        }

        c_hold_timer = 0.0f;
        c_hold_target_type = CHOLD_TARGET::NONE;
    }
}

shared_ptr<CPlayer> CPlayer::FindNearestOtherPlayer()
{
    auto room = GetRoom();
    if (!room) 
        return nullptr;

    CScene* scene = room->GetScenes()[(UINT)current_scene_type].get();
    if (!scene) 
        return nullptr;

    auto& players = scene->GetPlayers();
    shared_ptr<CPlayer> nearest = nullptr;
    float minDistSq = FLT_MAX;

    for (const auto& [id, player] : players) {

        if (!player || id == obj_id)
            continue;
        if (player->GetIsPossessed() || player->IsIncapacitated() || player->is_returned)
            continue;

        XMFLOAT3 dir = Vector3::Subtract(player->GetPosition(), position);
        dir.y = 0.0f;
        float distSq = dir.x * dir.x + dir.z * dir.z;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearest   = player;
        }
    }

    return nearest;
}

XMFLOAT3 CPlayer::GetRandomPossessedTarget()
{
    const float TILE_SIZE = 2.0f;
    int cx = (int)roundf(position.x / TILE_SIZE);
    int cz = (int)roundf(position.z / TILE_SIZE);

    const int dx[] = { 0, 0, -1, 1 };
    const int dz[] = { -1, 1,  0, 0 };

    std::vector<MapGenerator::Cell> candidates;
    for (int i = 0; i < 4; i++) {

        int nx = cx + dx[i];
        int nz = cz + dz[i];

        if (MapGenerator::IsWalkableFloor(nx, nz) && !MapGenerator::IsBlockedStructure(nx, nz))
            candidates.push_back({ nx, nz });
    }

    if (candidates.empty())
        return position;

    int idx = rand() % (int)candidates.size();
    return { candidates[idx].x * TILE_SIZE, position.y, candidates[idx].y * TILE_SIZE };
}

void CPlayer::ProcessVisibleObjectMining(const InputData& input, float elapsedTime, bool isMoving)
{
    // 채굴 시작: lbtn 탭 + 정지 + 착지 (채굴 중이면 재시작 금지: 진행 중 dig_timer 리셋으로 데미지 유실 방지)
    if (input.lbtn && dig_timer <= 0.0f && !isMoving && !is_possessed && !is_dowsing
        && grounded_timer > 0.0f) {
        state = PLAYER_STATE::DIG;
        dig_timer = DIG_DURATION;
        dig_sound_timer = 0.0f;
    }

    // 채굴 소리 타이머: DIG 시작 후 0.5초 시점에 PlaySound 패킷 전송
    if (dig_sound_timer >= 0.0f) {
        dig_sound_timer += elapsedTime;
        if (dig_sound_timer >= 0.5f) {
            dig_sound_timer = -1.0f;

            if (auto r = room.lock()) {
                auto* gameScene = static_cast<CGameScene*>(r->GetScenes()[(UINT)SCENE_TYPE::GAME].get());
                if (gameScene && gameScene->FindNearestMineable(position, CGameScene::MINING_RANGE, MINEABLEOBJECT_TYPE::VISIBLE)) {
                    SendSoundPacket(false, SOUND_ID::flying_pan, GetPosition());
                }
            }
        }
    }

    // 채굴 타이머 감소 (타이머가 살아있으면 DIG 유지, 만료 시 IDLE 전환 + 데미지)
    if (dig_timer > 0.0f) {

        dig_timer -= elapsedTime;

        if (dig_timer > 0.0f) {
            state = PLAYER_STATE::DIG;
        }
        else {
            state = PLAYER_STATE::IDLE;
            ProcessToolMining();
        }
    }
}

void CPlayer::ProcessUnVisibleObjectMining(const InputData& input, float elapsedTime, bool isMoving)
{
    bool isBareHand = (equipped_item_id == 0);
    bool isShovel = equipped_item_id >= 1 && equipped_item_id <= 4;

    // 채굴 시작: lbtn 탭 + 정지 + 착지 (채굴 중이면 재시작 금지: 진행 중 dig_timer 리셋으로 데미지 유실 방지)
    if (input.lbtn && isShovel && dig_timer <= 0.0f
        && !isMoving && !is_possessed && !is_dowsing
        && grounded_timer > 0.0f) {
        state = PLAYER_STATE::DIG;
        dig_timer = DIG_DURATION;
        dig_sound_timer = 0.0f;
    }

    // 채굴 소리 타이머: DIG 시작 후 0.5초 시점에 PlaySound 패킷 전송 
    if (dig_sound_timer >= 0.0f) {
        dig_sound_timer += elapsedTime;
        if (dig_sound_timer >= 0.5f) {
            dig_sound_timer = -1.0f;

            if (auto r = room.lock()) {
                auto* gameScene = static_cast<CGameScene*>(r->GetScenes()[(UINT)SCENE_TYPE::GAME].get());
                if (gameScene && gameScene->FindNearestMineable(position, CGameScene::BARE_HAND_MINING_RANGE, MINEABLEOBJECT_TYPE::NONE_VISIBLE)) {
                    SendSoundPacket(false, SOUND_ID::bare_hand_dig, GetPosition(), 0, 3.33f);
                }
            }
        }
    }

    // 채굴 타이머 감소 (타이머가 살아있으면 DIG 유지, 만료 시 IDLE 전환 + 데미지)
    if (isShovel && dig_timer > 0.0f) {

        dig_timer -= elapsedTime;

        if (dig_timer > 0.0f) {
            state = PLAYER_STATE::DIG;
        }
        else {
            ProcessToolMining();
            state = PLAYER_STATE::IDLE;
        }
    }

    if (isBareHand) {
        ProcessBareHandMining(elapsedTime, input, isMoving);
    }
}

void CPlayer::ProcessToolMining()
{
    if (auto r = room.lock()) {
        auto* gameScene = static_cast<CGameScene*>(r->GetScenes()[(UINT)SCENE_TYPE::GAME].get());
        if (gameScene) {
            bool isShovel = equipped_item_id >= 1 && equipped_item_id <= 4;

            CMineableObject* target = nullptr;
            if (!isShovel) {
                target = gameScene->FindNearestMineable(position, CGameScene::MINING_RANGE, MINEABLEOBJECT_TYPE::VISIBLE);
            }
            else {
                target = gameScene->FindNearestMineable(position, CGameScene::BARE_HAND_MINING_RANGE, MINEABLEOBJECT_TYPE::NONE_VISIBLE);
            }

            if (target) {
                target->TakeDamage();
                if (target->IsDestroyed()) {
                    if (isShovel)
                        gameScene->DestroyMineable(target->GetID(), position, look);
                    else
                        gameScene->DestroyMineable(target->GetID());
                }

                // 도구 내구도 소모 (0이면 자동 제거)
                ConsumeEquippedDurability();
            }
        }
    }
}

// 장착 장비의 내구도를 1단계 소모한다. 0이 되면 인벤토리에서 제거하고 장착 해제한다.
// 반환값: 내구도 0으로 파괴되어 제거됐으면 true
bool CPlayer::ConsumeEquippedDurability()
{
    if (!equipped_item)
        return false;

    auto equip = std::dynamic_pointer_cast<CEquipment>(equipped_item);
    if (!equip || equip->GetMaxDurability() == 0)
        return false;   // 내구도 개념이 없는 장비는 건너뜀

    equip->ReduceDurability(); 

    // 플레이어에게 내구도 상태 알려주기
    S_UpdateDurability updateDurPkt;
    updateDurPkt.player_id = GetID();
    updateDurPkt.item_id = equipped_item_id;
    updateDurPkt.inventory_id = equipped_item->GetInventoryID();
    updateDurPkt.current_durability = equip->GetCurrentDurability();
    updateDurPkt.item_type = equipped_item->GetItemType();
    updateDurPkt.item_sub_type = equipped_item_sub_type;
    updateDurPkt.scene_type = GetCurrentSceneType();

    if (GetSession()) {
        auto sendBuffer = MAKE_SEND_BUFFER(updateDurPkt);
        GetSession()->DoSend(sendBuffer);
    }

    // 내구도가 남아 있으면 종료
    if (equip->GetCurrentDurability() > 0)
        return false;

    // 내구도 0 → 인벤토리에서 제거
    if (!inventory || !inventory->RemoveItem(equipped_item->GetInventoryID()))
        return false;

    // 본인에게: 인벤토리에서 아이템 없애라고 지시
    S_RemoveItem removeItemPkt;
    removeItemPkt.item_id = equipped_item_id;
    removeItemPkt.inventory_id = equipped_item->GetInventoryID();
    removeItemPkt.player_id = GetID();
    removeItemPkt.scene_type = GetCurrentSceneType();

    if (GetSession()) {
        auto sendBuffer = MAKE_SEND_BUFFER(removeItemPkt);
        GetSession()->DoSend(sendBuffer);
    }

    // 전체에게: 해당 유저의 장착이 해제됐다고 알린다
    S_EquipItem equipPkt;
    equipPkt.player_id = GetID();
    equipPkt.item_id = 0;
    equipPkt.scene_type = GetCurrentSceneType();

    if (auto r = GetRoom()) {
        if (auto sc = r->GetScenes()[(UINT)current_scene_type].get()) {
            auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
            sc->BroadCast(sendBuffer);
        }
    }

    // 장착 해제
    equipped_item = nullptr;
    equipped_item_id = 0;
    equipped_item_sub_type = ITEM_SUB_TYPE::NONE;
    return true;
}

void CPlayer::ProcessBareHandMining(float elapsedTime, const InputData& input, bool isMoving)
{
    if (!input.lbtn_held || isMoving || is_possessed || is_dowsing || grounded_timer <= 0.0f) {
        bare_hand_dig_timer = 0.0f;
        bare_hand_sound_timer = -1.0f;
        state = PLAYER_STATE::IDLE;
        return;
    }

    if (auto r = room.lock()) {
        auto* gameScene = static_cast<CGameScene*>(r->GetScenes()[(UINT)SCENE_TYPE::GAME].get());
        if (gameScene) {
            CMineableObject* target = gameScene->FindNearestMineable(position, CGameScene::BARE_HAND_MINING_RANGE, MINEABLEOBJECT_TYPE::NONE_VISIBLE);

            if (target) {
                state = PLAYER_STATE::DIG;
                bare_hand_dig_timer += elapsedTime;

                // 채굴 사운드: 홀딩 중 일정 간격마다 PlaySound 패킷 전송 (시작 즉시 1회 재생)
                if (bare_hand_sound_timer < 0.0f)
                    bare_hand_sound_timer = BARE_HAND_SOUND_INTERVAL;

                bare_hand_sound_timer += elapsedTime;

                if (bare_hand_sound_timer >= BARE_HAND_SOUND_INTERVAL) {
                    bare_hand_sound_timer -= BARE_HAND_SOUND_INTERVAL;
                    SendSoundPacket(false, SOUND_ID::bare_hand_dig, GetPosition(), 0, 3.33f);
                }

                if (bare_hand_dig_timer >= 8.0f) {
                    bare_hand_dig_timer = 0.0f;
                    bare_hand_sound_timer = -1.0f;
                    state = PLAYER_STATE::IDLE;
                    target->DestroyImmediate();
                    if (target->IsDestroyed())
                        gameScene->DestroyMineable(target->GetID(), position, look);
                }
            }
            else {
                bare_hand_dig_timer = 0.0f;
                bare_hand_sound_timer = -1.0f;
            }
        }
    }
}

void CPlayer::ProcessMeleeAttack(const InputData& input, float elapsedTime)
{
    if (input.lbtn && !is_possessed && !is_dowsing
        && grounded_timer > 0.0f && melee_attack_timer <= 0.0f) {
        state = PLAYER_STATE::ATTACK;
        melee_attack_timer = MELEE_ATTACK_DURATION;
    }

    if (melee_attack_timer > 0.0f) {
        float prev = melee_attack_timer;
        melee_attack_timer -= elapsedTime;

        // 0.4초 경과 시 데미지 (1.5 - 0.4 = 1.1 threshold)
        if (prev > 1.1f && melee_attack_timer <= 1.1f)
            MeleeAttack();

        state = (melee_attack_timer > 0.0f) ? PLAYER_STATE::ATTACK : PLAYER_STATE::IDLE;
    }
}

void CPlayer::MeleeAttack()
{
    auto r = GetRoom();
    if (!r) 
        return;

    auto scene = r->GetScenes()[(UINT)current_scene_type].get();
    if (!scene) 
        return;

    XMFLOAT3 playerPos  = position;
    XMFLOAT3 playerLook = look;
    playerLook.y = 0.0f;
    float lookLen = sqrtf(playerLook.x * playerLook.x + playerLook.z * playerLook.z);
    if (lookLen < 0.001f) 
        return;

    playerLook.x /= lookLen;
    playerLook.z /= lookLen;

    XMFLOAT3 right = { playerLook.z, 0.0f, -playerLook.x };

    constexpr float MELEE_DEPTH      = 1.3f;
    constexpr float MELEE_HALF_WIDTH = 0.6f;

    // 장착 무기의 공격력 (없으면 기본 20)
    int weaponDamage = 20;
    if (auto weapon = std::dynamic_pointer_cast<CWeapon>(equipped_item)) {
        if (weapon->GetAttackPower() > 0)
            weaponDamage = static_cast<int>(weapon->GetAttackPower());
    }

    for (auto& [id, monster] : scene->GetMonsters()) {
        if (!monster)
            continue;

        MON_TYPE mType = monster->GetMonsterType();
        if (mType != MON_TYPE::HUMAN_MONSTER && mType != MON_TYPE::ANIMAL_MONSTER)
            continue;

        XMFLOAT3 toMonster = Vector3::Subtract(monster->GetPosition(), playerPos);
        toMonster.y = 0.0f;

        float forwardDist = playerLook.x * toMonster.x + playerLook.z * toMonster.z;
        if (forwardDist < 0.0f || forwardDist > MELEE_DEPTH)
            continue;

        float lateralDist = fabsf(right.x * toMonster.x + right.z * toMonster.z);
        if (lateralDist > MELEE_HALF_WIDTH)
            continue;

        monster->ApplyMeleeHit(playerPos, static_pointer_cast<CPlayer>(shared_from_this()), weaponDamage);

        // 몬스터에 맞았을 때만 무기 내구도 소모 (0이면 자동 제거)
        ConsumeEquippedDurability();
    }

    // 빙의된 플레이어 근접 공격: 넉백 + 스턴만 적용 (체력 감소 없음)
    for (auto& [id, player] : scene->GetPlayers()) {
        if (!player || id == obj_id)
            continue;
        if (!player->GetIsPossessed())
            continue;

        XMFLOAT3 toPlayer = Vector3::Subtract(player->GetPosition(), playerPos);
        toPlayer.y = 0.0f;

        float forwardDist = playerLook.x * toPlayer.x + playerLook.z * toPlayer.z;
        if (forwardDist < 0.0f || forwardDist > MELEE_DEPTH)
            continue;

        float lateralDist = fabsf(right.x * toPlayer.x + right.z * toPlayer.z);
        if (lateralDist > MELEE_HALF_WIDTH)
            continue;

        player->ApplyKnockback(toPlayer, 3.0f, 4.0f);
        SendSoundPacket(false, SOUND_ID::damaged1, player->GetPosition());
    }
}

void CPlayer::ProcessRangedAttack(const InputData& input, float elapsedTime)
{
    switch (equipped_item_id)
    {
        case 16:  // 스프레이
        {
            if (input.lbtn && !is_possessed && !is_dowsing
                && grounded_timer > 0.0f && ranged_attack_timer <= 0.0f) {
                SendSoundPacket(false, SOUND_ID::ghost_spray, GetPosition());
                state = PLAYER_STATE::ATTACK;
                ranged_attack_timer = SPRAY_ATTACK_DURATION;
            }

            if (ranged_attack_timer > 0.0f) {
                float prev = ranged_attack_timer;
                ranged_attack_timer -= elapsedTime;

                // 0.8초 경과 시 데미지 (1.5 - 0.8 = 0.7 threshold)
                if (prev > 0.7f && ranged_attack_timer <= 0.7f) {
                    SprayAttack(elapsedTime);

                    // 스프레이 내구도 소모 (0이면 자동 제거)
                    if (ConsumeEquippedDurability())
                        ranged_attack_timer = 0.0f;
                }

                state = (ranged_attack_timer > 0.0f) ? PLAYER_STATE::ATTACK : PLAYER_STATE::IDLE;
            }
        }
        break;
        case 17: // 마법 지팡이
        {
            if (input.lbtn && !is_possessed && !is_dowsing
                && grounded_timer > 0.0f && ranged_attack_timer <= 0.0f) {
                state = PLAYER_STATE::ATTACK;
                ranged_attack_timer = MAGIC_ATTACK_DURATION;
            }

            if (ranged_attack_timer > 0.0f) {
                float prev = ranged_attack_timer;
                ranged_attack_timer -= elapsedTime;

                state = (ranged_attack_timer > 0.0f) ? PLAYER_STATE::ATTACK : PLAYER_STATE::IDLE;
            }
        }
        break;
        case 18: // 비비탄총
        {
            if (input.lbtn && !is_possessed && !is_dowsing
                && grounded_timer > 0.0f && ranged_attack_timer <= 0.0f) {
                state = PLAYER_STATE::ATTACK;
                ranged_attack_timer = GUN_ATTACK_DURATION;
            }

            if (ranged_attack_timer > 0.0f) {
                float prev = ranged_attack_timer;
                ranged_attack_timer -= elapsedTime;

                state = (ranged_attack_timer > 0.0f) ? PLAYER_STATE::ATTACK : PLAYER_STATE::IDLE;
            }
        }
        break;
    }
}

void CPlayer::SprayAttack(float elapsedTime)
{
    constexpr float SPRAY_RANGE = 1.5f;

    auto r = GetRoom();
    if (!r) 
        return;

    auto scene = r->GetScenes()[(UINT)current_scene_type].get();
    if (!scene) 
        return;

    XMFLOAT3 playerPos  = position;
    XMFLOAT3 playerLook = look;

    for (auto& [id, monster] : scene->GetMonsters()) {
        if (!monster || monster->GetMonsterType() != MON_TYPE::GHOST)
            continue;
        if (monster->GetAIState() == AI_STATE::MONSTER_FLEE)
            continue;

        XMFLOAT3 toMonster = Vector3::Subtract(monster->GetPosition(), playerPos);
        toMonster.y = 0.0f;
        float dist = Vector3::Length(toMonster);

        if (dist > SPRAY_RANGE || dist < 0.001f)
            continue;

        // 전방 체크: 플레이어 정면 방향에 있는지
        float dot = playerLook.x * (toMonster.x / dist) + playerLook.z * (toMonster.z / dist);
        if (dot <= 0.0f)
            continue;

        static_cast<CGhost*>(monster.get())->ApplySprayHit(playerPos, static_pointer_cast<CPlayer>(shared_from_this()));
    }

    // 빙의된 플레이어 스프레이 히트
    for (auto& [id, player] : scene->GetPlayers()) {
        if (!player || id == obj_id)
            continue;
        if (!player->GetIsPossessed())
            continue;

        XMFLOAT3 toPlayer = Vector3::Subtract(player->GetPosition(), playerPos);
        toPlayer.y = 0.0f;
        float dist = Vector3::Length(toPlayer);

        if (dist > SPRAY_RANGE || dist < 0.001f)
            continue;

        float dot = playerLook.x * (toPlayer.x / dist) + playerLook.z * (toPlayer.z / dist);
        if (dot <= 0.0f)
            continue;

        player->ApplySprayHitWhilePossessed(playerPos);
    }
}

void CPlayer::ApplySprayHitWhilePossessed(const XMFLOAT3& fromPos)
{
    constexpr int   MAX_SPRAY_HITS  = 3;
    constexpr float KNOCKBACK_FORCE = 3.0f;

    XMFLOAT3 awayDir = Vector3::Subtract(position, fromPos);
    ApplyKnockback(awayDir, KNOCKBACK_FORCE, 0.0f);

    SendSoundPacket(false, SOUND_ID::devil_scared1, GetPosition());

    possessed_spray_hit_count++;

    if (possessed_spray_hit_count >= MAX_SPRAY_HITS) {
        SendSoundPacket(false, SOUND_ID::devil_laugh1, GetPosition());
        possessed_spray_hit_count = 0;
        is_possessed     = false;
        possession_timer = 0.0f;
    }
}

void CPlayer::RecordServerFrameHistory(const ServerFrameHistory& history)
{
	server_history_deq.push_back(history);

	if (server_history_deq.size() > RENDER_BUFFER_MAX_SIZE)
		server_history_deq.pop_front();
}

// 나중에 유저간의 충돌 처리를 할 경우 사용될 함수.
bool CPlayer::FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult)
{
    if (server_history_deq.empty()) return false;

    // 1. 범위를 벗어난 요청 처리 (너무 오래됐거나 너무 최신인 경우)
    if (targetTime <= server_history_deq.front().timestamp)
    {
        outResult = server_history_deq.front();
        return true;
    }
    if (targetTime >= server_history_deq.back().timestamp)
    {
        outResult = server_history_deq.back();
        return true;
    }

    // 2. 이진 탐색으로 targetTime보다 크거나 같은 첫 번째 원소 찾기
    auto it = std::lower_bound(server_history_deq.begin(), server_history_deq.end(), targetTime,
        [](const ServerFrameHistory& frame, float time) {
            return frame.timestamp < time;
        });

    if (it == server_history_deq.begin() || it == server_history_deq.end())
    {
        outResult = *it;
        return true;
    }

    // 3. targetTime을 사이에 둔 두 프레임 확보 (it는 B, it-1은 A)
    const ServerFrameHistory& frameB = *it;
    const ServerFrameHistory& frameA = *(std::prev(it));

    // 4. 두 지점 사이를 보간하여 "그때 그 순간"의 좌표 계산
    float timeDiff = frameB.timestamp - frameA.timestamp;
    float alpha = 0.0f;
    if (timeDiff > 0.0f)
        alpha = (targetTime - frameA.timestamp) / timeDiff;

    // 결과 조립
    outResult.timestamp = targetTime;
    outResult.position = Vector3::Lerp(frameA.position, frameB.position, alpha);

    // 상태나 입력값은 보간이 불가능하므로 이전 프레임(A)의 것을 따름
    outResult.state = frameA.state;
    outResult.seq_num = frameA.seq_num;

    return true;
}

void CPlayer::UpdateStamina(float elapsedTime)
{
    if (current_scene_type == SCENE_TYPE::LOBBY)
        return;

    const float drainPerSec = 16.7f;        // 뛸 때 초당 감소 (6초면 바닥)
    const float regenPerSec = 15.0f;        // 쉴 때 초당 회복 
    const float recoverThreshold = 30.0f;   // 이 값 이상 회복돼야 다시 달리기 허용

    if (start_jump) {
        constexpr float jumpCost = 12.0f;
        accumulate_stamina -= jumpCost;
        if (accumulate_stamina <= 0.0f) {
            accumulate_stamina = 0.0f;
            stamina_exhausted = true;
        }
    }

    if (state == PLAYER_STATE::RUN) {

        accumulate_stamina -= drainPerSec * elapsedTime;

        if (accumulate_stamina <= 0.0f) {

            accumulate_stamina = 0.0f;
            stamina_exhausted = true;

            if (auto move = GetComponent<CMovementComponent>())
                move->UnRun();
        }
    }
    else {
        if (!is_grounded || state == PLAYER_STATE::ATTACK)
            return;

        accumulate_stamina += regenPerSec * elapsedTime;

        if (accumulate_stamina > static_cast<float>(stat.maxStamina))
            accumulate_stamina = static_cast<float>(stat.maxStamina);

        if (stamina_exhausted && accumulate_stamina >= recoverThreshold)
            stamina_exhausted = false;
    }

    stat.stamina = static_cast<uint32>(accumulate_stamina);
}

void CPlayer::ResetAll()
{
    bool was_dowsing = is_dowsing;
    is_possessed = false;
    possession_timer = 0.0f;
    is_dowsing = false;
    is_stunned = false;
    stun_timer = 0.0f;
    is_returned = false;
    is_ready = false;
    state = PLAYER_STATE::IDLE;
    stat.hp = 100;
    stat.stamina = 100;
    accumulate_stamina = 100;
    stamina_exhausted = false;
    equipped_item = nullptr;
    equipped_item_id = 0;
    equipped_item_sub_type = ITEM_SUB_TYPE::NONE;

    // 모든 collider mask 복구 (DEAD에서 0으로 만들었던 것)
    // plr가 여러 collider를 가지므로 PhysicsManager에서 일괄 처리
    if (auto r = room.lock()) {
        if (auto pm = r->GetScenes()[(UINT)current_scene_type]->GetPhysicsManager()) {
            uint32_t plrMask = static_cast<uint32_t>(
                EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND |
                EColLayer::PLAYER | EColLayer::CHARACTER);
            pm->SetMaskByOwner(this, plrMask);
        }
    }

    S_EquipItem equipPkt;
    equipPkt.is_dowsing_rod = false;
    equipPkt.item_id = was_dowsing ? -1 : 0;
    equipPkt.scene_type = current_scene_type;
    equipPkt.player_id = GetID();
    auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
    if (auto session = GetSession()) {
        session->DoSend(sendBuffer);
    }
}

void CPlayer::AddStamina(uint32 amount)
{
    accumulate_stamina = min(accumulate_stamina + static_cast<float>(amount),
        static_cast<float>(stat.maxStamina));

    stat.stamina = static_cast<uint32>(accumulate_stamina);
    if (stamina_exhausted && accumulate_stamina >= 30.0f)
        stamina_exhausted = false;
}

void CPlayer::SendPing()
{
    S_Ping pingPkt;
    pingPkt.server_send_time = g_server_total_time; // 현재 서버 누적 시간

    if (session.lock()) {
        auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Ping>(pingPkt);
        session.lock()->DoSend(sendBuffer);
    }
}