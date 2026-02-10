#pragma once
#include "Character.h"

class CCamera;

struct OpponentFrameHistory 
{
	uint64	 player_id;
	float	 server_timestamp; // 서버에서 찍어준 도장
	XMFLOAT3 position;
	PLAYER_STATE state;
};


class CPlayer : public CCharacter {
public:
	CPlayer();
	virtual void Update(float elapsedTime) override;
	void PreUpdate(float elapsedTime);

	void SetState(const PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetDestInfo(const ObjectInfo& pos) { dest_info = pos; }
	void RecordOpponentFrameHistory(const OpponentFrameHistory& state);

	bool GetIsMyPlayer() const { return is_my_player; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

private:
	void OpponentMoveSyncByInterpolation(float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

protected:
	uint32 room_id; // 이 플레이어가 참여하고 있는 방 ID
	SCENE_TYPE current_scene_type; // 현재 플레이어가 속한 씬 (방이 씬을 포함하고 있는 구조)

	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	
	ObjectInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값

	float smoothed_delay = 0.1f;

	std::deque<OpponentFrameHistory> interpolation_deq;
};

