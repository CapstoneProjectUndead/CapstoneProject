#pragma once
// ServerÂÊ Player

#include "Object.h"

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(float elapsedTime) override;

public:
	uint64_t	last_processed_seq = 0;
	InputData	current_input;
};

