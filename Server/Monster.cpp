#include "pch.h"
// Server쪽 Monster
#include "Monster.h"
#include "Scene.h"
#include "GameScene.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "Item.h"
#include "AIComponent.h"
#include "State.h"

CMonster::CMonster(MON_TYPE type)
	: CObject(OBJECT_TYPE::MONSTER)
	, monster_type(type)
	, AI_state(AI_STATE::MONSTER_IDLE)
    , current_scene(nullptr)
    , idle_timer(0.f)
    , patrol_timer(0.f)
    , attack_timer(0.f)
    , turn_timer(0.f)
    , respawn_time(20.f)
{
}

CMonster::~CMonster()
{
    if (auto r = room.lock()) {
        if (auto physicsManager = r->GetScenes()[(UINT)current_scene_type]->GetPhysicsManager()) {
            physicsManager->EraseCollider(GetComponent<CColliderComponent>());
        }
    }
}

void CMonster::Update(float elapsedTime)
{
	last_simulated_time = static_cast<float>(g_server_total_time);

    auto nearest = FindNearestPlayer();
    if (nearest) {
        constexpr float SLEEP_DIST_SQ = 20.0f * 20.0f;
        XMFLOAT3 diff = Vector3::Subtract(nearest->GetPosition(), position);
        if (diff.x * diff.x + diff.z * diff.z > SLEEP_DIST_SQ)
            return;
    }

	CObject::Update(elapsedTime);
}

void CMonster::ApplySeparation(float scale)
{
    constexpr float SEPARATION_RADIUS = 1.5f;
    constexpr float SEPARATION_FORCE  = 4.0f;

    auto r = GetRoom();
    if (!r) return;
    auto scene = r->GetScenes()[(UINT)current_scene_type].get();
    auto gameScene = dynamic_cast<CGameScene*>(scene);
    if (!gameScene) return;

    XMFLOAT3 push = { 0.f, 0.f, 0.f };
    for (const auto& info : gameScene->GetMonsterSpawnInfo()) {
        if (info.type != monster_type) continue;
        auto other = info.monster.lock();
        if (!other || other.get() == this) continue;

        XMFLOAT3 diff = Vector3::Subtract(position, other->GetPosition());
        diff.y = 0.0f;
        float dist = Vector3::Length(diff);
        if (dist > 0.01f && dist < SEPARATION_RADIUS) {
            float invDist  = 1.0f / dist;
            float strength = (SEPARATION_RADIUS - dist) / SEPARATION_RADIUS;
            push.x += diff.x * invDist * strength * SEPARATION_FORCE;
            push.z += diff.z * invDist * strength * SEPARATION_FORCE;
        }
    }
    velocity.x += push.x * scale;
    velocity.z += push.z * scale;
}

shared_ptr<CPlayer> CMonster::FindNearestPlayer()
{
    if (auto room = GetRoom()) {
        CScene* scene = room->GetScenes()[(UINT)current_scene_type].get();
        auto& players = scene->GetPlayers();

        shared_ptr<CPlayer> nearest_player = nullptr;
        float min_dist_sq = FLT_MAX; // 최소 거리를 찾기 위해 float 최대값으로 초기화

        for (const auto& pair : players) {

            auto player = pair.second;
            if (!player)
                continue;

            // 추가 조건: 플레이어가 죽었거나 빙의 상태라면 무시
            if (player->GetState() == PLAYER_STATE::DEAD)
                continue;
            if (player->GetIsPossessed())
                continue;

            // MathHelper를 활용해 직관적으로 두 좌표의 차이 벡터를 구함
            XMFLOAT3 dir = Vector3::Subtract(player->GetPosition(), position);

            // Vector3::Distance 대신 직접 제곱합을 구해 루트 연산(sqrt) 오버헤드 방지
            float dist_sq = (dir.x * dir.x) + (dir.y * dir.y) + (dir.z * dir.z);

            if (dist_sq < min_dist_sq) {
                min_dist_sq = dist_sq;
                nearest_player = player;
            }
        }

        return nearest_player;
    }

    return nullptr;
}

void CMonster::ApplyMeleeHit(const XMFLOAT3& fromPos, shared_ptr<CPlayer> player)
{
    XMFLOAT3 awayDir = Vector3::Subtract(position, fromPos);
    awayDir.y = 0.0f;
    float len = Vector3::Length(awayDir);
    if (len < 0.001f) 
        return;

    target_player = player;

    if (monster_type == MON_TYPE::HUMAN_MONSTER) {
        SendSoundPacket(false, SOUND_ID::surprising_girl, GetPosition());
    }
    else if (monster_type == MON_TYPE::ANIMAL_MONSTER) {
        SendSoundPacket(false, SOUND_ID::dog_moan, GetPosition());
    }

    melee_knockback_vel   = { awayDir.x / len * MELEE_KNOCKBACK_FORCE, 0.0f, awayDir.z / len * MELEE_KNOCKBACK_FORCE };
    melee_knockback_timer = MELEE_KNOCKBACK_DURATION;
    velocity.x = melee_knockback_vel.x;
    velocity.z = melee_knockback_vel.z;

    auto* ai = GetComponent<CAIComponent>();
    if (ai) {
        auto cur = ai->GetCurrentState();
        if (!cur || cur->GetType() != AI_STATE::MONSTER_TRACE)
            ai->ChangeState(AI_STATE::MONSTER_TRACE);
    }
}

void CMonster::PatrolRadiusWander(float elapsedTime)
{
    const float arrive_threshold     = 0.3f;
    const float stuck_check_interval = 0.1f;
    const float stuck_threshold      = 0.03f;

    if (is_waiting) {
        velocity.x = 0.0f;
        velocity.z = 0.0f;
        wander_wait_timer += elapsedTime;
        if (wander_wait_timer >= wander_wait_duration) {
            wander_target        = GetRandomWanderTarget();
            wander_wait_duration = 1.0f + (rand() % 11) * 0.1f;
            is_waiting           = false;
            wander_wait_timer    = 0.0f;
            stuck_check_timer    = 0.0f;
            last_stuck_pos       = position;
        }
        return;
    }

    stuck_check_timer += elapsedTime;
    if (stuck_check_timer >= stuck_check_interval) {
        XMFLOAT3 moved = Vector3::Subtract(position, last_stuck_pos);
        moved.y = 0.0f;
        if (Vector3::Length(moved) < stuck_threshold)
            wander_target = GetRandomWanderTarget();
        stuck_check_timer = 0.0f;
        last_stuck_pos    = position;
    }

    XMFLOAT3 dirVec = Vector3::Subtract(wander_target, position);
    dirVec.y = 0.0f;
    float dist = Vector3::Length(dirVec);

    if (dist < arrive_threshold) {
        velocity.x        = 0.0f;
        velocity.z        = 0.0f;
        is_waiting        = true;
        wander_wait_timer = 0.0f;
        return;
    }

    float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
    SetYaw(targetYaw);
    SetYawPitch(targetYaw, 0.0f);

    velocity.x = look.x * patrol_speed;
    velocity.z = look.z * patrol_speed;
}

XMFLOAT3 CMonster::GetRandomWanderTarget()
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

void CMonster::DropItem(uint16 itemID)
{
    auto item = current_scene->GetItemManager()->SpawnItem(itemID, GetPosition());
    S_SpawnItem spawnPkt;
    spawnPkt.item_id = item->item->GetItemId();
    spawnPkt.item_type = item->item->GetItemType();
    spawnPkt.item_world_id = item->world_id;
    spawnPkt.scene_type = current_scene_type;
    spawnPkt.x = item->position.x;
    spawnPkt.y = 0.04f;
    spawnPkt.z = item->position.z;

    auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
    current_scene->BroadCast(sendBuffer);
}
