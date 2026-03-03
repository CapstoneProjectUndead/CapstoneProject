#pragma once
#include "LightManager.h"

class CPlayer;
class CMyPlayer;
class CCamera;
class CObject;
class CShader;
class CObjectFactory;

class CScene
{
public:
	CScene(SCENE_TYPE type);
	~CScene();

	void ReleaseUploadBuffers();

	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) abstract;

	void AnimateObjects(float);

	virtual void Initialize();
	virtual void Render(ID3D12GraphicsCommandList*);
	virtual void Update(float elapsedTime);

	// Scene 이 전환될 때, 호출 될 함수
	virtual void Enter() abstract;
	virtual void Exit();

	void EnterScene(std::shared_ptr<CObject>, UINT);
	void LeaveScene(UINT);

	// UI 관련 
	void DrawUI_Final();

protected:
	// UI 관련 
	// 자식들이 각자 그릴 UI를 구현하는 순수 가상 함수
	virtual void DrawUI() = 0;
	virtual bool IsUIInputEnabled() = 0;

private:
	// UI 관련 
	void ManageIME(); 
	bool last_input_state = true; 

public:
	// 서버 패킷 관련 처리 함수들
	void Handle_S_Spawn_Player(std::shared_ptr<Session>& session, const S_SpawnPlayer& pkt);
	void Handle_S_PLAYER_LIST(S_PLAYER_LIST& pkt);
	void Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_Move& pkt);
	void Handle_S_Remove_Player(std::shared_ptr<Session>& session, const S_RemovePlayer& pkt);

public:
	// 멤버 변수 set
	std::shared_ptr<CMyPlayer>				GetMyPlayer() const { return my_player; }
	void									SetPlayer(std::shared_ptr<CMyPlayer> _player) { my_player = _player; }
	void									SetCamera(std::shared_ptr<CCamera> _camera) { camera = _camera; }
	std::shared_ptr<CCamera>&				GetCamera() { return camera; }

	SCENE_TYPE								GetSceneType() const { return scene_type; }

	auto&									GetShaders() { return shaders; }
	void									SetShaders(auto& otherShaders) { shaders = otherShaders; }
	std::vector<std::shared_ptr<CObject>>&	GetObjects() { return objects; }
	std::unordered_map<uint64, size_t>&     GetIDIndex() { return id_To_Index; }

	void									SetLight(std::unique_ptr<CLightManager> _light) { light = std::move(_light); }

	std::shared_ptr<CObjectFactory>& GetFactory() { return factory; };
protected:
	SCENE_TYPE								scene_type;

	std::unordered_map<std::string, std::shared_ptr<CShader>>	shaders{};
	std::shared_ptr<CMyPlayer>				my_player;			// 내 플레이어
	std::shared_ptr<CCamera>				camera;

	std::vector<std::shared_ptr<CObject>>	objects;			// 다른 플레이어 or 몬스터 or 오브젝트
	std::unordered_map<uint64, size_t>	    id_To_Index;

	std::unique_ptr<CLightManager> light;
	std::shared_ptr<CObjectFactory> factory;
};