#pragma once

class CObject;

class CComponent {
public:
	virtual void Initialize() {}
	virtual void Update(const float deltaTime) = 0;

	virtual void Render(ID3D12GraphicsCommandList* commandList) {};
	virtual void ReleaseUploadBuffer() {}
	// Object 단위
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* commandList) {}
	// Mesh 단위
	virtual void UpdateMeshShaderVariables(ID3D12GraphicsCommandList* commandList) {}
	virtual void CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {}

	CObject* GetOwner() const { return owner; }
	void SetEnable(const bool e) { is_enable = e; }

	CObject* owner{};	// 참조용
	bool is_enable{ true };
};
