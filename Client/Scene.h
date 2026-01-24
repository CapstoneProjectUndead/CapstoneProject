#pragma once
#include "Shader.h"
#include "GeometryLoader.h"
#include "LightManager.h"

class CPlayer;
class CMyPlayer;
class CCamera;

class CScene
{
public:
	CScene() = default;
	~CScene() = default;

	void ReleaseUploadBuffers();

	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) abstract;

	void AnimateObjects(float);

	// 멤버 변수 set
	virtual void Render(ID3D12GraphicsCommandList*);
	virtual void Update(float elapsedTime);

	void EnterScene(std::shared_ptr<CObject>, UINT);
	void LeaveScene(UINT);

	std::shared_ptr<CMyPlayer>				GetMyPlayer() const { return my_player; }
	void									SetPlayer(std::shared_ptr<CMyPlayer> _player) { my_player = _player; }
	void									SetCamera(std::shared_ptr<CCamera> _camera) { camera = _camera; }

	std::vector<std::shared_ptr<CShader>>&	GetShaders() { return shaders; }
	std::vector<std::shared_ptr<CObject>>&	GetObjects() { return objects; }

protected:
	std::vector<std::shared_ptr<CShader>>	shaders{};
	std::shared_ptr<CMyPlayer>				my_player;			// 내 플레이어
	std::shared_ptr<CCamera>				camera;

	std::vector<std::shared_ptr<CObject>>	objects;			// 다른 플레이어 or 오브젝트
	std::unordered_map<uint32_t, size_t>	id_To_Index;

	std::unique_ptr<CLightManager> light;
};