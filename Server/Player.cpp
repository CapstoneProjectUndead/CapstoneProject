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


CPlayer::CPlayer()
	: CObject(OBJECT_TYPE::PLAYER)
    , last_processed_seq(0)
    , ping(0.0f)
    , dt_ping_accumulator(0.0f)
	, state(PLAYER_STATE::IDLE)
    , equipped_item_id(0)
    , equipped_item_sub_type(ITEM_SUB_TYPE::NONE)
    , is_ready(false)
    , accumulate_stamina(100.f)
    , stamina_exhausted(false)
    , dig_timer(0.f)
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
        InputData emptyInput{ false, false, false, false, false, false };
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
        CObject::Update(elapsedTime);
        return;
    }

    // --------------------
    // 입력 처리 및 방향 계산
    // --------------------
    XMFLOAT3 dir{ 0.f, 0.f, 0.f };
    if (input.w) dir.z++;
    if (input.s) dir.z--;
    if (input.a) dir.x--;
    if (input.d) dir.x++;

    // 점프
    if (input.space) {
        if (auto move = GetComponent<CMovementComponent>())
            move->Jump();
    }

    // 상태 갱신
    bool isMoving = (dir.x != 0 || dir.z != 0);

    // 움찔거리는 거 방지 (Client PredictMove와 동일 로직)
    grounded_timer = is_grounded ? 0.1f : (grounded_timer - elapsedTime);

    if (updateState) {

        ProcessMining(input, elapsedTime, isMoving);

        if (grounded_timer > 0.0f) {

            if (!isMoving) {

                if (state != PLAYER_STATE::DIG)
                    state = PLAYER_STATE::IDLE;

                if (is_grounded && knockback_timer <= 0.0f) {
                    velocity.x = 0.0f;
                    velocity.z = 0.0f;
                }
            }
            else {
                // 이동 입력이 들어오면 채굴 취소
                dig_timer = 0.0f;
                if (auto move = GetComponent<CMovementComponent>()) {
                    if (input.shift && !stamina_exhausted) {
                        state = PLAYER_STATE::RUN;
                        move->Run();
                    }
                    else {
                        state = PLAYER_STATE::WALK;
                        move->UnRun();
                    }
                }
            }
        }
        else {
            dig_timer = 0.0f;
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

    if (knockback_timer <= 0.0f && stun_timer <= 0.0f && (dir.x != 0 || dir.z != 0)) {
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
        if (stun_timer < 0.0f)
            stun_timer = 0.0f;
    }

    // Movement와 Collider Update
    CObject::Update(elapsedTime);
}

void CPlayer::ApplyKnockback(XMFLOAT3 dir, float force)
{
    dir.y = 0.0f;
    float len = Vector3::Length(dir);

    if (len < 0.001f) 
        return;

    knockback_vel = { dir.x / len * force, 0.0f, dir.z / len * force };
    knockback_timer = 0.3f;
    ApplyStun(1.5f);
}

void CPlayer::ApplyStun(float time)
{
    stun_timer = time;
    state = PLAYER_STATE::IDLE;
}

void CPlayer::ApplyPossession()
{
    is_possessed    = true;
    possession_timer = 30.0f;

    possessed_nav_path.clear();
    possessed_path_refresh_timer = 0.0f;
    possessed_wander_target      = GetRandomPossessedTarget();
    possessed_is_waiting         = false;
    possessed_wait_timer         = 0.0f;
    possession_contact_timer     = 0.0f;
}

void CPlayer::UpdatePossession(float elapsedTime)
{
    possession_timer        -= elapsedTime;
    possession_contact_timer += elapsedTime;

    if (possession_timer <= 0.0f) {
        possession_timer = 0.0f;
        is_possessed     = false;
        velocity.x       = 0.0f;
        velocity.z       = 0.0f;
        state            = PLAYER_STATE::IDLE;
        return;
    }

    constexpr float TILE_SIZE     = 2.0f;
    constexpr float ARRIVE_DIST   = 0.4f;
    constexpr float POSSESS_SPEED = 4.0f;

    auto nearOther = FindNearestOtherPlayer();

    if (nearOther) {
        XMFLOAT3 dirVec = Vector3::Subtract(nearOther->GetPosition(), position);
        dirVec.y = 0.0f;
        float dist = Vector3::Length(dirVec);

        if (dist > 0.001f) {
            float targetYaw = XMConvertToDegrees(atan2f(dirVec.x, dirVec.z));
            SetYaw(targetYaw);
            SetYawPitch(targetYaw, 0.0f);
            velocity.x = look.x * POSSESS_SPEED;
            velocity.z = look.z * POSSESS_SPEED;
            state      = PLAYER_STATE::RUN;
        }

        auto* myCol    = GetComponent<CColliderComponent>();
        auto* otherCol = nearOther->GetComponent<CColliderComponent>();
        if (myCol && otherCol && myCol->Intersects(otherCol)) {
            if (possession_contact_timer >= 1.0f) {
                uint32 hp = nearOther->GetHp();
                nearOther->SetHp(hp > 15 ? hp - 15 : 0);
                possession_contact_timer = 0.0f;

                XMFLOAT3 knockbackDir = Vector3::Subtract(nearOther->GetPosition(), position);
                nearOther->ApplyKnockback(knockbackDir, 0.8f);
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

shared_ptr<CPlayer> CPlayer::FindNearestOtherPlayer()
{
    auto r = GetRoom();
    if (!r) 
        return nullptr;

    CScene* scene = r->GetScenes()[(UINT)current_scene_type].get();
    if (!scene) 
        return nullptr;

    auto& players = scene->GetPlayers();
    shared_ptr<CPlayer> nearest = nullptr;
    float minDistSq = FLT_MAX;

    for (const auto& [id, p] : players) {

        if (!p || id == obj_id) continue;
        if (p->GetIsPossessed()) continue;

        XMFLOAT3 dir = Vector3::Subtract(p->GetPosition(), position);
        dir.y = 0.0f;
        float distSq = dir.x * dir.x + dir.z * dir.z;

        if (distSq < minDistSq) {
            minDistSq = distSq;
            nearest   = p;
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

void CPlayer::ProcessMining(const InputData& input, float elapsedTime, bool isMoving)
{
    // 채굴 시작: lbtn 클릭 + 정지 + 착지 + 도구 장착 + Game 씬
    if (input.lbtn && !isMoving && grounded_timer > 0.0f) {

        if (equipped_item_id != 0 
            && equipped_item_sub_type == ITEM_SUB_TYPE::TOOL 
            && current_scene_type == SCENE_TYPE::GAME) {

            state = PLAYER_STATE::DIG;
            dig_timer = DIG_DURATION;
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

            if (auto r = room.lock()) {
                auto* gameScene = static_cast<CGameScene*>(r->GetScenes()[(UINT)SCENE_TYPE::GAME].get());
                if (gameScene) {

                    CMineableObject* target = gameScene->FindNearestMineable(position, CGameScene::MINING_RANGE);

                    if (target) {
                        target->TakeDamage();
                        if (target->IsDestroyed())
                            gameScene->DestroyMineable(target->GetID());

                        if (equipped_item && equipped_item->GetSubType() == ITEM_SUB_TYPE::TOOL) {
                            auto tool = std::static_pointer_cast<CTool>(equipped_item);

                            // 내구도 닳기
                            tool->ReduceDurability();

                            // 플레이어에게 내구도 상태 알려주기
                            S_UpdateDurability updateDurPkt;
                            updateDurPkt.player_id = GetID();
                            updateDurPkt.item_id = equipped_item_id;
                            updateDurPkt.inventory_id = equipped_item->GetInventoryID();
                            updateDurPkt.current_durability = tool->GetCurrentDurability();
                            updateDurPkt.item_type = equipped_item->GetItemType();
                            updateDurPkt.item_sub_type = equipped_item_sub_type;
                            updateDurPkt.scene_type = GetCurrentSceneType();

                            if (GetSession()) {
                                auto sendBuffer = MAKE_SEND_BUFFER(updateDurPkt);
                                GetSession()->DoSend(sendBuffer);
                            }

                            // 내구도가 0인지 체크
                            if (tool->GetCurrentDurability() <= 0) {

                                // 인벤토리에서 도구 제거
                                if (inventory && inventory->RemoveItem(equipped_item->GetInventoryID())) {

                                    // 내구도 0이라서 플레이어에게 해당 아이템 없애라고 지시
                                    S_RemoveItem removeItemPkt;
                                    removeItemPkt.item_id = equipped_item_id;
                                    removeItemPkt.inventory_id = equipped_item->GetInventoryID();
                                    removeItemPkt.player_id = GetID();
                                    removeItemPkt.scene_type = GetCurrentSceneType();

                                    if (GetSession()) {
                                        auto sendBuffer = MAKE_SEND_BUFFER(removeItemPkt);
                                        GetSession()->DoSend(sendBuffer);
                                    }

                                    // 해당 유저와 다른 유저들에게 해당 유저가 들고있던 무기 없어졌다고 알려야한다.
                                    S_EquipItem equipPkt;
                                    equipPkt.player_id = GetID();
                                    equipPkt.item_id = 0;
                                    equipPkt.scene_type = GetCurrentSceneType();

                                    auto sendBuffer = MAKE_SEND_BUFFER(equipPkt);
                                    gameScene->BroadCast(sendBuffer);

                                    // 장착 해제
                                    equipped_item = nullptr;
                                    equipped_item_id = 0;
                                    equipped_item_sub_type = ITEM_SUB_TYPE::NONE;
                                }
                            }
                        }
                    }
                }
            }
        }
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
    const float drainPerSec = 16.7f;        // 뛸 때 초당 감소 (6초면 바닥)
    const float regenPerSec = 10.0f;        // 쉴 때 초당 회복 
    const float recoverThreshold = 30.0f;   // 이 값 이상 회복돼야 다시 달리기 허용

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
        accumulate_stamina += regenPerSec * elapsedTime;

        if (accumulate_stamina > static_cast<float>(stat.maxStamina))
            accumulate_stamina = static_cast<float>(stat.maxStamina);

        if (stamina_exhausted && accumulate_stamina >= recoverThreshold)
            stamina_exhausted = false;
    }

    stat.stamina = static_cast<uint32>(accumulate_stamina);
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