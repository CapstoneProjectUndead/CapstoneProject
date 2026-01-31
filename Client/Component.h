#pragma once

class CObject;

class CComponent {
public:
	virtual void Initialize() {}
	virtual void Update(const float deltaTime) = 0;

	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList) {}
	virtual void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {}

	CObject* owner{};	// 참조용
};
