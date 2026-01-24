#pragma once
#include "Character.h"

class CCamera;

class CPlayer : public CCharacter {
public:
	using CObject::Move; // 부모의 Move 모두 노출

	CPlayer();
	void Move(const XMFLOAT3 shift) override;
protected:
	float friction{ 125.0f };
};

