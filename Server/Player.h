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

class CUser;
class CRoom;

class CPlayer : public CObject
{
public:
	CPlayer();
	~CPlayer();

	void Update(const float elapsedTime) override;
	void ProcessInputQueue(const float elapsedTime);
	void SimulateMove(const InputData& input, float deltaTime);

	// 최대 속도 제한
	void ClampSpeed();
	void Slide(const XMFLOAT3& normal);
	void Slide(const XMVECTOR& normal);

	void SetLastSequence(uint64 lastSeq) { last_processed_seq = lastSeq; }
	uint64 GetLastSequence() const { return last_processed_seq; }

	void SetState(PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetCurrentSceneType(SCENE_TYPE type) { current_scene_type = type; }
	SCENE_TYPE GetCurrentSceneType() const { return current_scene_type; }

	std::weak_ptr<CUser>      GetUserWeak() const { return user; }
	std::shared_ptr<CUser>    GetUser() const { return user.lock(); }
	void SetUser(shared_ptr<CUser> _user) { user = _user; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

	shared_ptr<CRoom> GetRoom() const { return room; }
	void SetRoom(shared_ptr<CRoom> _room) { room = _room; }

	void RecordServerFrameHistory(const ServerFrameHistory& history);
	bool FindHistoryAtTime(float targetTime, ServerFrameHistory& outResult);

	deque<ServerFrameHistory>& GetFrameHistoryDeq() { return server_history_deq; }
	void PushInput(const PendingInput& input) { input_queue.push_back(input); }

	float GetLastSimulatedTime() const { return last_simulated_time; }

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
	uint32						room_id; // 이 플레이어가 참여하고 있는 방 ID
	SCENE_TYPE					current_scene_type; // 현재 플레이어가 속한 씬 (방이 씬을 포함하고 있는 구조)
	weak_ptr<CUser>				user;
	shared_ptr<CRoom>			room;
	uint64						last_processed_seq;
	float						last_simulated_time;
	float						ping;
	float						dt_ping_accumulator;
	PLAYER_STATE				state;
	deque<PendingInput>			input_queue;
	deque<ServerFrameHistory>	server_history_deq;
};

