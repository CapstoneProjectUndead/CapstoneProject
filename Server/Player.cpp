#include "pch.h"
// Server쪽 Player
#include "Player.h"

CPlayer::CPlayer()
	: last_processed_seq(0)
	, server_timestamp(0.0f)
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
	server_timestamp = static_cast<float>(g_server_total_time);

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

            // 매 시뮬레이션 직후 장부 기록 (패킷 하나하나의 결과 기록)
            ServerFrameHistory frame{};
            frame.input = pending.input;
            frame.seq_num = last_processed_seq;
            frame.position = position;
            frame.state = state;
            frame.timestamp = static_cast<float>(server_timestamp);

            RecordServerFrameHistory(frame);
        }
    }
    else
    {
        // 패킷이 안 온 프레임이라면 빈 입력을 넣어 관성/마찰만 적용
        InputData emptyInput{ false, false, false, false };
        SimulateMove(emptyInput, elapsedTime);
    }

	// 회전 Update
	SetYawPitch(yaw, pitch);
	UpdateWorldMatrix();
}

void CPlayer::SimulateMove(const InputData& input, float deltaTime)
{
    // 1. 방향 계산 (입력이 없으면 dir은 0, 0, 0)
    XMFLOAT3 dir{ 0.f, 0.f, 0.f };
    if (input.w) dir.z++;
    if (input.s) dir.z--;
    if (input.a) dir.x--;
    if (input.d) dir.x++;

    // 2. 가속도 적용 (dir이 0이면 가속도도 0이 되어 속도에 영향을 주지 않음)
    if (Vector3::Length(dir) > 0.0f)
    {
        XMFLOAT3 accel{};

        if (dir.z > 0) accel = Vector3::Add(accel, look);
        if (dir.z < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(look, -1));
        if (dir.x < 0) accel = Vector3::Add(accel, Vector3::ScalarProduct(right, -1));
        if (dir.x > 0) accel = Vector3::Add(accel, right);

        // 가속도 적용: velocity += accel * speed * deltaTime
        velocity = Vector3::Add(velocity, Vector3::ScalarProduct(accel, speed * deltaTime));

        state = PLAYER_STATE::WALK;
    }
    else
    {
        state = PLAYER_STATE::IDLE;
    }

    // 3. 최대 속도 제한
    float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (lenXZ > max_speed) {
        float ratio = max_speed / lenXZ;
        velocity.x *= ratio;
        velocity.z *= ratio;
    }

    // 4. 실제 위치 이동
    position = Vector3::Add(position, Vector3::ScalarProduct(velocity, deltaTime));

    // 5. 감속(마찰) 적용
    float speedLen = Vector3::Length(velocity);
    float decel = friction * deltaTime;
    if (decel > speedLen) decel = speedLen;

    velocity = Vector3::Add(velocity, Vector3::ScalarProduct(velocity, -decel, true));
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
