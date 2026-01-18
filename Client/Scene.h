#pragma once
#include "Shader.h"

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

	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) abstract;

	void AnimateObjects(float);

	// 멤버 변수 set
	virtual void Render(ID3D12GraphicsCommandList*);
	virtual void Update(float elapsedTime);

	std::shared_ptr<CPlayer> GetMyPlayer() const { return player; }
	void SetPlayer(std::shared_ptr<CPlayer> _player) { player = _player; }

	void SetCamera(CCamera* _camera) { camera = _camera; }

	std::vector<std::shared_ptr<CPlayer>>& GetOtherPlayers() { return objects; }

protected:
	std::vector<std::shared_ptr<CShader>> shaders{};
	std::shared_ptr<CPlayer> player;	// 내 플레이어
	CCamera* camera = nullptr;	// 참조용

	std::vector<std::shared_ptr<CPlayer>> objects; // 다른 플레이어 or 오브젝트

	ComPtr<ID3D12RootSignature> graphics_root_signature{};
};