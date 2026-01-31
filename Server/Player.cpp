#include "pch.h"
// Server쪽 Player
#include "Player.h"

CPlayer::CPlayer()
	: last_processed_seq(0)
	, total_simulation_time(0.0f)
	, current_input{}
	, state(PLAYER_STATE::IDLE)
{

}

CPlayer::~CPlayer()
{

}

void CPlayer::Update(const float elapsedTime)
{
	total_simulation_time += elapsedTime;

    UpdateMovement(elapsedTime);

	// 회전 Update
	SetYawPitch(yaw, pitch);
	UpdateWorldMatrix();
}

void CPlayer::UpdateMovement(const float elapsedTime)
{
    // 최대 속도 제한
    float lenXZ = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (lenXZ > max_speed) {
        float ratio = max_speed / lenXZ;
        velocity.x *= ratio;
        velocity.z *= ratio;
    }

    // 이동
    position = Vector3::Add(position, Vector3::ScalarProduct(velocity, elapsedTime));

    // 감속(마찰)
    float speedLen = Vector3::Length(velocity);
    float decel = friction * elapsedTime;
    if (decel > speedLen) decel = speedLen;

    velocity = Vector3::Add(velocity, Vector3::ScalarProduct(velocity, -decel, true));
}

void CPlayer::SimulateMove(const InputData& input, float dt)
{
	XMFLOAT3 dir{ 0.f, 0.f, 0.f };
	if (input.w) dir.z++;
	if (input.s) dir.z--;
	if (input.a) dir.x--;
	if (input.d) dir.x++;

	// 상태 update
	if (dir.x == 0 && dir.z == 0)
		state = PLAYER_STATE::IDLE;
	else
		state = PLAYER_STATE::WALK;

	if (dir.x != 0 || dir.z != 0) {
		Move(dir, dt);
	}
}

void CPlayer::RecordFrameHistory(const ServerFrameHistory& history)
{
	history_deq.push_back(history);

	if (history_deq.size() > 600)
		history_deq.pop_front();
}

bool CPlayer::FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult)
{
    if (history_deq.empty()) return false;

    // 1. 범위를 벗어난 요청 처리 (너무 오래됐거나 너무 최신인 경우)
    if (targetTime <= history_deq.front().timestamp)
    {
        outResult = history_deq.front();
        return true;
    }
    if (targetTime >= history_deq.back().timestamp)
    {
        outResult = history_deq.back();
        return true;
    }

    // 2. 이진 탐색으로 targetTime보다 크거나 같은 첫 번째 원소 찾기
    auto it = std::lower_bound(history_deq.begin(), history_deq.end(), targetTime,
        [](const ServerFrameHistory& frame, float time) {
            return frame.timestamp < time;
        });

    if (it == history_deq.begin() || it == history_deq.end())
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