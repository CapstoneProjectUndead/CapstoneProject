#pragma once
// ServerÂÊ Player

#include "Object.h"

struct ServerFrameHistory
{
	uint64		 seq_num;
	InputData	 input;
	XMFLOAT3	 position;
	PLAYER_STATE state;
	float		 timestamp; 
};

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(const float elapsedTime) override;
	void SimulateMove(const InputData& input, float dt);

	void SetLastSequence(uint64 lastSeq) { last_processed_seq = lastSeq; }
	uint64 GetLastSequence() const { return last_processed_seq; }

	void SetInput(const InputData& input) { current_input = input; }
	InputData GetInput() const { return current_input; }

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void RecordFrameHistory(const ServerFrameHistory& history);
	bool FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult);

	deque<ServerFrameHistory>& GetFrameHistoryDeq() { return history_deq; }

	float GetTotalSimulationTime() const { return total_simulation_time; }

private:
	uint64						last_processed_seq;
	float						total_simulation_time;
	InputData					current_input;
	PLAYER_STATE				state;
	deque<ServerFrameHistory>	history_deq;
};

