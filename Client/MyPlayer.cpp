#include "stdafx.h"
#include "MyPlayer.h"
#include "Timer.h"
#include "KeyManager.h"

CMyPlayer::CMyPlayer()
	: CPlayer()
{
	
}

void CMyPlayer::Update(float elapsedTime)
{
	// 입력처리
	ProcessInput();

	// 여기서 플레이어 위치와 방향 정보를 캐싱
	{
		C_Move movePkt;
		movePkt.info.x = position.x;
		movePkt.info.y = position.y;
		movePkt.info.z = position.z;
	}
}

void CMyPlayer::ProcessInput()
{
	XMFLOAT3 direction{};

	if (KEY_PRESSED(KEY::W)) direction.z++;
	if (KEY_PRESSED(KEY::S)) direction.z--;
	if (KEY_PRESSED(KEY::A)) direction.x--;
	if (KEY_PRESSED(KEY::D)) direction.x++;

	if (direction.x != 0 || direction.z != 0) {
		Move(direction, CTimer::GetInstance().GetTimeElapsed());
	}

	CKeyManager& keyManager{ CKeyManager::GetInstance() };

	if (KEY_PRESSED(KEY::LBTN) || KEY_PRESSED(KEY::RBTN)) {
		SetCursor(NULL);
		Vec2 prevMousePos{ keyManager.GetPrevMousePos() };
		Vec2 mouseDelta{ (keyManager.GetMousePos() - prevMousePos) / 3.0f };
		if (mouseDelta.x || mouseDelta.y)
		{
			if (KEY_PRESSED(KEY::LBTN))
				Rotate(mouseDelta.y, mouseDelta.x, 0.0f);
			if (KEY_PRESSED(KEY::RBTN))
				Rotate(mouseDelta.y, 0.0f, -mouseDelta.x);
		}
	}
}