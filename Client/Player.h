#pragma once
#include "Character.h"

class CCamera;

struct OpponentState 
{
	float	 serverTimestamp; // 서버에서 찍어준 도장
	XMFLOAT3 position;
};


class CPlayer : public CCharacter {
public:
	CPlayer();
	virtual void Update(float elapsedTime) override;

	void SetState(const PLAYER_STATE _state) { state = _state; }
	PLAYER_STATE GetState() const { return state; }

	void SetDestInfo(const ObjectInfo& pos) { dest_info = pos; }
	void PushOpponentState(const OpponentState& state) { interpolation_deq.push_back(state); }

private:
	void OpponentMoveSync(const float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

	// 
	void OpponentMoveSyncByInterpolation(float dt);

protected:
	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	
	ObjectInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값

	std::deque<OpponentState> interpolation_deq;
};

