#pragma once
#include "Object.h"

class CCamera;

class CPlayer : public CObject{
public:
	CPlayer(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
	void Update(float elapsedTime) override;
	void Move(const XMFLOAT3 shift) override;
	void Move(const XMFLOAT3 direction, float distance) override;

	std::shared_ptr<CCamera> GetCamera() const { return camera; }
	void OnUpdateTransform();

	//
	XMFLOAT3 position{ XMFLOAT3(0.0f, 0.0f, 0.0f) };
	XMFLOAT3 right{ XMFLOAT3(1.0f, 0.0f, 0.0f) };
	XMFLOAT3 up{ XMFLOAT3(0.0f, 1.0f, 0.0f) };
	XMFLOAT3 look{ XMFLOAT3(0.0f, 0.0f, 1.0f) };
protected:
	std::shared_ptr<CCamera> camera;
	float friction{ 125.0f };
	XMFLOAT3 velocity{1.0f, 1.0f, 1.0f};
};

