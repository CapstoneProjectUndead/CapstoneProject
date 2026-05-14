#include "pch.h"
// Server쪽 Monster
#include "Monster.h"
#include "Scene.h"
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
    SendSoundPacket(false, SOUND_ID::surprising_girl, GetPosition());

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

void CMonster::DropItem(uint16 itemID)
{
    auto item = current_scene->GetItemManager()->SpawnItem(itemID, GetPosition());
    S_SpawnItem spawnPkt;
    spawnPkt.item_id = item->item->GetItemId();
    spawnPkt.item_type = item->item->GetItemType();
    spawnPkt.item_world_id = item->world_id;
    spawnPkt.scene_type = current_scene_type;
    spawnPkt.x = item->position.x;
    spawnPkt.y = 0.2f;
    spawnPkt.z = item->position.z;

    auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
    current_scene->BroadCast(sendBuffer);
}
