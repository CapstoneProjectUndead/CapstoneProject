#pragma once
// ServerÂÊ Player

#include "Object.h"

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(const float elapsedTime) override;

	void PredictMove(const InputData& input, float dt);

	void SetLastSequence(uint64 lastSeq) { last_processed_seq = lastSeq; }
	uint64 GetLastSequence() const { return last_processed_seq; }

	void SetInput(const InputData& input) { current_input = input; }
	InputData GetInput() const { return current_input; }

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

private:
	uint64	last_processed_seq = 0;
	InputData	current_input;
	PLAYER_STATE state = PLAYER_STATE::IDLE;
};

