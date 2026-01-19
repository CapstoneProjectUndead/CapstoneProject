#pragma once
// ServerÂÊ Player

#include "Object.h"

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(float elapsedTime) override;
};

