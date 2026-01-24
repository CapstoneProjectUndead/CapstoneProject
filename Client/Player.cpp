#include "stdafx.h"
#include "Player.h"

// Player
CPlayer::CPlayer()
	: CCharacter()
{
	is_visible = true;
	SetPosition(XMFLOAT3(0.0f, 0.0f, 0.0f));
}

void CPlayer::Move(const XMFLOAT3 shift)
{
	position = Vector3::Add(position, shift);
}