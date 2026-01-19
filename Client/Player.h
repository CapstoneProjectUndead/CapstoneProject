#pragma once
#include "Object.h"

class CCamera;

class CPlayer : public CObject{
public:
	using CObject::Move; // 부모의 Move 모두 노출

	CPlayer();
	virtual void Update(float elapsedTime) override;
	void Move(const XMFLOAT3 shift) override;
protected:
	float friction{ 125.0f };
};

