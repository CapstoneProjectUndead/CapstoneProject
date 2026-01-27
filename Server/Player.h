#pragma once
// ServerÂÊ Player

#include "Object.h"

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(float elapsedTime) override;

	void PredictMove(const InputData& input, float dt);

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

public:
	uint64_t	last_processed_seq = 0;
	InputData	current_input;
	PLAYER_STATE state = PLAYER_STATE::IDLE;
};

