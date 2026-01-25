#pragma once
#include "Character.h"

class CCamera;

class CPlayer : public CCharacter {
public:
	CPlayer();
protected:
	float friction{ 125.0f };
};

