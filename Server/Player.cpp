#include "pch.h"
// Server쪽 Player
#include "Player.h"

CPlayer::CPlayer()
{

}

CPlayer::~CPlayer()
{

}

void CPlayer::Update(const float elapsedTime)
{
	PredictMove(current_input, elapsedTime);

	// 회전 입력
	SetYawPitch(yaw, pitch);
	UpdateWorldMatrix();
}

void CPlayer::PredictMove(const InputData& input, float dt)
{
	XMFLOAT3 dir{ 0.f, 0.f, 0.f };
	if (input.w) dir.z++;
	if (input.s) dir.z--;
	if (input.a) dir.x--;
	if (input.d) dir.x++;

	if (dir.x != 0 || dir.z != 0) {
		Move(dir, dt);
	}
}