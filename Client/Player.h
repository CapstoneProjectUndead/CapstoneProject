#pragma once
#include "Object.h"

class CCamera;

class CPlayer : public CObject {
public:
	using CObject::Move; // 부모의 Move 모두 노출

	CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	virtual void Update(float elapsedTime) override;
	void Move(const XMFLOAT3 shift) override;

	void SetDestPos(const XMFLOAT3& pos) { dest_pos = pos; }

protected:
	float friction{ 125.0f };
	XMFLOAT3 velocity{1.0f, 1.0f, 1.0f};

	bool is_my_player = false;	// 크게 필요없을 것 같지만, 일단 선언
	XMFLOAT3 dest_pos{};
};

