#pragma once
#include "Character.h"

class CCamera;

class CPlayer : public CCharacter {
public:
	CPlayer();
	virtual void Update(float elapsedTime) override;

	void SetDestInfo(const ObjectInfo& pos) { dest_info = pos; }

private:
	void OpponentMoveSync(const float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

protected:
	float friction{ 125.0f };
	XMFLOAT3 direction{};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	// 크게 필요없을 것 같지만, 일단 선언
	ObjectInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값
};

