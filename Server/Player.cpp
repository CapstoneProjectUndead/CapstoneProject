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

	// 회전 Update
	SetYawPitch(yaw, pitch);
	UpdateWorldMatrix();
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
