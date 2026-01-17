#pragma once
#include "Object.h"

class CCamera;

class CPlayer : public CObject{
public:
	using CObject::Move; // 부모의 Move 모두 노출

	CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void Update(float elapsedTime) override;
	void Move(const XMFLOAT3 shift) override;

	void ProcessInput();

	CCamera* GetCameraPtr() const { return camera.get(); }
protected:
	std::shared_ptr<CCamera> camera;
	float friction{ 125.0f };
	XMFLOAT3 velocity{1.0f, 1.0f, 1.0f};
};

