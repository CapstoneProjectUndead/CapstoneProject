#pragma once
#include "Shader.h"

class CPlayer;
class CMyPlayer;
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

	void EnterScene(std::shared_ptr<CObject>, UINT);
	void LeaveScene(UINT);

	std::shared_ptr<CMyPlayer>	GetMyPlayer() const { return my_player; }
	void						SetPlayer(std::shared_ptr<CMyPlayer> _player) { my_player = _player; }
	void						SetCamera(CCamera* _camera) { camera = _camera; }

	std::vector<std::shared_ptr<CObject>>& GetObjects() { return objects; }
	std::unordered_map<uint32_t, size_t>& GetIDIndex() { return id_To_Index; }

protected:
	std::vector<std::shared_ptr<CShader>>	shaders{};
	std::shared_ptr<CMyPlayer>				my_player;			// 내 플레이어
	CCamera*								camera = nullptr;	// 참조용

	std::vector<std::shared_ptr<CObject>>	objects;			// 다른 플레이어 or 오브젝트
	std::unordered_map<uint32_t, size_t>	id_To_Index;

	ComPtr<ID3D12RootSignature> graphics_root_signature{};
};