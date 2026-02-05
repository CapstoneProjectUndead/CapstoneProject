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

private:
	void OpponentMoveSyncByInterpolation(float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

protected:
	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	
	ObjectInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값

	std::deque<OpponentFrameHistory> interpolation_deq;
};

