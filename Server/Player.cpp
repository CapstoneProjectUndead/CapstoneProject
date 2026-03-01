#include "pch.h"
// Server쪽 Player
#include "Player.h"
#include "Collider.h"
#include "PhysicsManager.h"

CPlayer::CPlayer()
	: room_id(-1)
    , current_scene_type(SCENE_TYPE::TITLE)
    , last_processed_seq(0)
	, last_simulated_time(0.0f)
    , ping(0.0f)
    , dt_ping_accumulator(0.0f)
	, state(PLAYER_STATE::IDLE)
{

}

CPlayer::~CPlayer()
{

}

void CPlayer::Update(const float elapsedTime)
{
    last_simulated_time = static_cast<float>(g_server_total_time);

    // 회전 Update
    SetYawPitch(yaw, pitch);
    UpdateWorldMatrix();
    ProcessInputQueue(elapsedTime);
}

void CPlayer::ProcessInputQueue(const float elapsedTime) // elapsedTime == g_targetDT(16.6ms)
{
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
        // 입력이 없어도 마찰/중력 계산을 위해 1회 업데이트
        InputData emptyInput{ false, false, false, false };
        SimulateMove(emptyInput, elapsedTime);

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

void CPlayer::SimulateMove(const InputData& input, float deltaTime)
{
    // -----------------------
    // 1. 입력 처리 및 방향 계산
    // -----------------------
    XMFLOAT3 dir{ 0.f, 0.f, 0.f };
    if (input.w) dir.z++;
    if (input.s) dir.z--;
    if (input.a) dir.x--;
    if (input.d) dir.x++;

    // 점프
    if (input.space && is_grounded) {
        velocity.y = jump_power;
    }

    // 상태 갱신
    if (dir.x == 0 && dir.z == 0) {
        state = PLAYER_STATE::IDLE;

        //velocity.x = 0.0f;
        //velocity.z = 0.0f;
    }
    else {
        state = PLAYER_STATE::WALK;
    }

    XMFLOAT3 accel{};

    if (dir.z > 0) accel = Vector3::Add(accel, look);
    if (dir.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(look, -1));
    if (dir.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(right, -1));
    if (dir.x > 0) accel = Vector3::Add(accel, right);

    accel = Vector3::Normalize(accel);

    // 가속도 적용: velocity += accel * speed * deltaTime
    velocity = Vector3::Add(velocity, Vector3::ScalarProduct(accel, speed * deltaTime));

    // 최대 속도 제한
    ClampSpeed();

    // 중력/마찰/땅 확인
    XMVECTOR groundSeparation = CPhysicsManager::GetInstance().ApplyGravity(this, deltaTime);

    // 이동량 계산 = 기존 위치 + 바닥 보정 + 외부 발판
    XMVECTOR finalPos = XMLoadFloat3(&position) + groundSeparation + CalculatePlatform(deltaTime);

    // 플레이어 이동량 계산
    XMVECTOR internalMotion = XMLoadFloat3(&velocity) * deltaTime;

    // 반복 슬라이딩 + 턱 오르기 수행
    ResolveCollisions(finalPos, internalMotion, deltaTime);

    // 최종 위치 적용
    XMStoreFloat3(&position, finalPos);

    auto collider = GetComponent<CColliderComponent>();
    if (collider) {
        UpdateWorldMatrix();
        collider->Update(deltaTime);
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

void CPlayer::SendPing()
{
    S_Ping pingPkt;
    pingPkt.server_send_time = g_server_total_time; // 현재 서버 누적 시간

    if (session.lock()) {
        auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Ping>(pingPkt);
        session.lock()->DoSend(sendBuffer);
    }
}

void CPlayer::ClampSpeed()
{
    float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (lenXZ > max_speed) {
        float ratio = max_speed / lenXZ;
        velocity.x *= ratio;
        velocity.z *= ratio;
    }
}

void CPlayer::Slide(const XMFLOAT3& normal)
{
    XMVECTOR v = XMLoadFloat3(&velocity);
    XMVECTOR n = XMLoadFloat3(&normal);

    XMVECTOR dot = XMVector3Dot(v, n);

    XMVECTOR result = v - n * dot;

    XMStoreFloat3(&velocity, result);
}

void CPlayer::Slide(const XMVECTOR& normal)
{
    XMVECTOR v = XMLoadFloat3(&velocity);

    XMVECTOR dot = XMVector3Dot(v, normal);

    XMVECTOR result = v - normal * dot;

    XMStoreFloat3(&velocity, result);
}

XMVECTOR CPlayer::CalculatePlatform(float dt)
{
    CollisionInfo info{};
    XMFLOAT3 downDelta = { 0, -0.1f, 0 };
    if (CPhysicsManager::GetInstance().Overlap(this, downDelta, info, EColLayer::GROUND | EColLayer::OBJECT)) {
        if (info.other_object && Vector3::Length(velocity) > 0.001f) {
            return XMLoadFloat3(&velocity) * dt;
        }
    }
    return XMVectorZero();
}

void CPlayer::ResolveCollisions(XMVECTOR& outPos, XMVECTOR remainingMotion, float dt)
{
    // 벽/오브젝트 충돌 처리
    uint32_t wallMask = EColLayer::WALL | EColLayer::OBJECT;
    const float stepHeight = 0.2; // 오를 수 있는 최대 높이

    // Iterative Slide
    for (int i = 0; i < 3; ++i) {
        float moveLen = XMVectorGetX(XMVector3Length(remainingMotion));
        if (moveLen < 0.0001f) break; // 더 이상 갈 거리가 없으면 종료

        CollisionInfo info{};
        if (CPhysicsManager::GetInstance().Overlap(this, Vector3::XMVectorToFloat3(remainingMotion), info, wallMask)) {
            // 1. step up
            if (TryStepUp(outPos, remainingMotion, info, stepHeight, wallMask)) {
                break;
            }
            // 2. slide
            float safeMargin = (info.depth > moveLen) ? moveLen : info.depth;   // depth 값이 비정상적일 때, 순간이동 방지
            safeMargin = max(safeMargin, 0.001f); // 최소한의 밀어내기 확보

            float normalLen = XMVectorGetX(XMVector3Length(info.normal));
            if (normalLen > 0.0001f) {
                XMVECTOR separation = -info.normal * safeMargin;
                outPos += separation;

                Slide(info.normal);

                // 남은 이동량 투영
                XMVECTOR newMotion = remainingMotion - info.normal * XMVector3Dot(remainingMotion, info.normal);

                // [추가 고려] 투영된 벡터가 원래 운동 방향과 너무 동떨어지거나 뒤로 가려고 하면 0으로 처리
                if (XMVectorGetX(XMVector3Dot(newMotion, remainingMotion)) <= 0) {
                    remainingMotion = XMVectorZero();
                }
                else {
                    remainingMotion = newMotion;
                }
            }
        }
        else {
            outPos += remainingMotion;
            break;
        }
    }
}

bool CPlayer::TryStepUp(XMVECTOR& outPos, XMVECTOR motion, const CollisionInfo& hit, float height, uint32_t mask)
{
    if (!is_grounded || std::abs(XMVectorGetY(hit.normal)) >= 0.2f) return false;

    // 캐릭터를 stepHeight만큼 위로 올린 임시 위치 계산
    XMVECTOR stepUpOffset = XMVectorSet(0, height, 0, 0);
    XMVECTOR testPos = outPos + stepUpOffset;

    XMFLOAT3 originalPos = position;
    position = Vector3::XMVectorToFloat3(testPos);   // 잠시 위치 이동

    CollisionInfo stepInfo{};
    bool obstructed = CPhysicsManager::GetInstance().Overlap(this, Vector3::XMVectorToFloat3(motion), stepInfo, mask);

    position = originalPos; // 복구

    if (!obstructed) {
        // 앞 공간이 비어있다면, 이제 다시 아래로 내려서 바닥이 있는지 확인
        // 실제 엔진은 여기서 아래로 Sweep을 쏘지만, 일단은 위치 확정 후 중력이 해결하게 함
        outPos = testPos + motion;
        return true;
    }
    return false;
}