#include "stdafx.h"
#include "Player.h"

extern HWND ghWnd;

// Player
CPlayer::CPlayer()
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CPlayer::Update(float elapsedTime)
{
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
}