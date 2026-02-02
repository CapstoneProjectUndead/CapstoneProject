#pragma once
// Server쪽 Player

#include "Object.h"

struct ServerFrameHistory
{
	uint64		 seq_num;
	InputData	 input;
	XMFLOAT3	 position;
	PLAYER_STATE state;
	float		 timestamp; 
};

struct PendingInput 
{
	InputData	input;
	uint64		seq_num;

	PendingInput() = default;
	PendingInput(InputData& _input, uint64 seqNum)
		: input(_input)
		, seq_num(seqNum)
	{ }
};

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(const float elapsedTime) override;
	void SimulateMove(const InputData& input, float deltaTime);

	void SetLastSequence(uint64 lastSeq) { last_processed_seq = lastSeq; }
	uint64 GetLastSequence() const { return last_processed_seq; }

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void RecordServerFrameHistory(const ServerFrameHistory& history);
	bool FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult);

	deque<ServerFrameHistory>& GetFrameHistoryDeq() { return server_history_deq; }
	deque<PendingInput>& GetInputQueue() { return input_queue; }
	void PushInput(const PendingInput& input) { input_queue.push_back(input); }

	float GetServerTime() const { return server_timestamp; }

	// 클라의 핑 측정
	void  SendPing();
	float GetLastPingSendTime() const { return dt_ping_accumulator; };
	void  SetLastPingSendTime(float elapsedTime) { dt_ping_accumulator = elapsedTime; };
	void  UpdatePing(float newRtt)
	{
		// 급격한 변화를 막기 위해 기존 값과 섞음 (보통 9:1 비율)
		ping = (ping * 0.9f) + (newRtt * 0.1f);
	}

private:
	uint64						last_processed_seq;
	float						server_timestamp;
	float						ping;
	float						dt_ping_accumulator;
	PLAYER_STATE				state;
	deque<PendingInput>			input_queue;
	deque<ServerFrameHistory>	server_history_deq;
};

