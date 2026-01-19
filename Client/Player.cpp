#include "stdafx.h"
#include "Player.h"
#include "Camera.h"
#include "Timer.h"
#include "KeyManager.h"

extern HWND ghWnd;

// 임시
XMFLOAT3 VInterpTo(XMFLOAT3& current, XMFLOAT3& target, float deltaTime, float interpSpeed)
{
	if (interpSpeed <= 0.f)
		return current;

	XMFLOAT3 delta = Vector3::Subtract(target, current);

	float scale = deltaTime * interpSpeed;

	// 스케일 클램프
	if (scale > 1.f) scale = 1.f;

	delta.x *= scale;
	delta.y *= scale;
	delta.z *= scale;

	return Vector3::Multiply(current, delta);
}

// Player
CPlayer::CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));

	// 메쉬 설정
	std::shared_ptr<CMesh> mesh = std::make_shared<CCubeMesh>(device, commandList);
	SetMesh(mesh);
}

void CPlayer::Update(float elapsedTime)
{
	//Move(velocity);

	if (false == is_my_player)
	{
		// 1. 서버에서 받은 목표 위치
		XMFLOAT3 serverPos{ dest_pos.x, dest_pos.y, dest_pos.z };

		float dist = Vector3::Distance(serverPos, position);

		// 2. 오차가 너무 크면 즉시 스냅
		if (dist > 3.f)
		{
			SetPosition(serverPos);
		}
		// 3. 오차가 작으면 부드럽게 보정
		else if (dist > 1.f)
		{
			XMFLOAT3 newPos = VInterpTo(
				position,
				serverPos,
				CTimer::GetInstance().GetTimeElapsed(),
				8.f
			);

			SetPosition(newPos);
		}

		SetPosition(serverPos);
	}

	// 감속
	XMVECTOR xmvVelocity = XMLoadFloat3(&velocity);
	XMVECTOR xmvDeceleration = XMVector3Normalize(XMVectorScale(xmvVelocity, -1.0f));
	float length = XMVectorGetX(XMVector3Length(xmvVelocity));
	float deceleration = friction * elapsedTime;
	if (deceleration > length) deceleration = length;
	XMStoreFloat3(&velocity, XMVectorAdd(xmvVelocity, XMVectorScale(xmvDeceleration, deceleration)));
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
	//if (camera) camera->Move(shift);
}