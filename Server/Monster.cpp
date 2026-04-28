#include "pch.h"
// Server쪽 Monster
#include "Monster.h"
#include "Scene.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"

CMonster::CMonster(MON_TYPE type)
	: CObject(OBJECT_TYPE::MONSTER)
	, monster_type(type)
	, AI_state(AI_STATE::MONSTER_IDLE)
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