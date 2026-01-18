#pragma once
#include "Shader.h"
#include "GeometryLoader.h"

class CPlayer;
class CCamera;

class CScene
{
public:
	CScene() = default;
	~CScene() = default;

	void ReleaseUploadBuffers();

	// 루트 시그니처
	ID3D12RootSignature* GetGraphicsRootSignature() { return graphics_root_signature.Get(); }
	virtual ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*);

	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*);

	void AnimateObjects(float);
	virtual void ProcessInput();

	// 멤버 변수 set
	virtual void Render(ID3D12GraphicsCommandList*);
protected:
	std::vector<std::shared_ptr<CShader>> shaders{};
	std::shared_ptr<CPlayer> player;
	CCamera* camera;	// 참조용

	ComPtr<ID3D12RootSignature> graphics_root_signature{};
	std::unordered_map<std::string, std::unique_ptr<FrameNode>> frames;
};