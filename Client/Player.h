#pragma once
#include "Object.h"

class CCamera;

class CPlayer : public CObject {
public:
	using CObject::Move; // 부모의 Move 모두 노출

	CPlayer();
	virtual void Update(float elapsedTime) override;
	void Move(const XMFLOAT3 shift) override;

	void SetDestInfo(const ObjectInfo& pos) { dest_info = pos; }

private:
	void OpponentMoveSync(float elapsedTime);
	void OpponentRotateSync(float elapsedTime);

protected:
	float friction{ 125.0f };
	XMFLOAT3 direction{};
	XMFLOAT3 velocity{1.0f, 1.0f, 1.0f};

	PLAYER_STATE state = PLAYER_STATE::IDLE;
	bool is_my_player = false;	// 크게 필요없을 것 같지만, 일단 선언
	ObjectInfo dest_info{};		// 서버로부터 받은 캐릭터의 위치, 회전 값
};

