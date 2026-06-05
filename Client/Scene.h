#pragma once
#include "LightManager.h"
#include "ImGuiManager.h"

class CPlayer;
class CMyPlayer;
class CCamera;
class CObject;
class CShader;
class CObjectFactory;
class CUIManager;

class CScene
{
public:
	CScene(SCENE_TYPE type);
	~CScene();

	void ReleaseUploadBuffers();

	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) abstract;

	void AnimateObjects(float);

	virtual void Initialize();
	void CollectObjects(ID3D12GraphicsCommandList* commandList);
	virtual void Render(ID3D12GraphicsCommandList*);
	virtual void Update(float elapsedTime);

	// Scene 이 전환될 때, 호출 될 함수
	virtual void Enter();
	virtual void Exit();

	void AddObject(std::shared_ptr<CObject>, UINT);
	void RemoveObject(UINT);

	void RemoveAllMonsters();

	// UI 관련 
	void DrawUI_Final();

	// UI 버튼 누를 때 효과음
	void CheckHoverSound();
	void PlayClickSound();

	// 상태변환
	void TransitionDepthBuffer(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);
	// main buffer set
	void SetGBufferRenderTargets(ID3D12GraphicsCommandList* commandList);
	// main buffer: RENDER_TARGET -> SR
	void TransitionGBuffersToSRV(ID3D12GraphicsCommandList* commandList);
	void SetBackBufferRenderTarget(ID3D12GraphicsCommandList* commandList);
	void SetBackBufferWithDepthReadOnly(ID3D12GraphicsCommandList* commandList);
	// Rendering
	void RenderDeferred(ID3D12GraphicsCommandList* commandList, ID3D12Resource* depthStencilBuf);
private:
	D3D12_RESOURCE_BARRIER CreateResourceBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);
	// Rendering
	void RenderBasePass(ID3D12GraphicsCommandList* commandList);
	void RenderShadowPass(ID3D12GraphicsCommandList* commandList);
	void RenderSSAOPass(ID3D12GraphicsCommandList* commandList);
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
	void Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_PlayerMove& pkt);
	void Handle_S_Remove_Player(std::shared_ptr<Session>& session, const S_RemovePlayer& pkt);
	void Handle_S_Spawn_Monster(std::shared_ptr<Session>& session, const S_SpawnMonster& pkt);
	void Handle_S_DeSpawn_Monster(std::shared_ptr<Session>& session, const S_DeSpawnMonster& pkt);
	void Handle_S_Move_Monster(std::shared_ptr<Session>& session, const S_MonsterMove& pkt);
	void Handle_S_Scene_Change(std::shared_ptr<Session>& session, const S_SceneChange& pkt);

	virtual void Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt) {}
	virtual void Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt) {}
	virtual void Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt) {}
	virtual void Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt) {}
	virtual void Handle_S_AddItemList(std::shared_ptr<Session> session, S_AddItemList& pkt) {}
	virtual void Handle_S_RemoveItem(std::shared_ptr<Session> session, const S_RemoveItem& pkt) {}
	virtual void Handle_S_EquipItem(std::shared_ptr<Session>& session, const S_EquipItem& pkt) {};
	virtual void Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt) {};
	virtual void Handle_S_MineableList(std::shared_ptr<Session>& session, S_MineableList& pkt) {};
	virtual void Handle_S_DestroyMineable(std::shared_ptr<Session>& session, const S_DestroyMineable& pkt) {};
	virtual void Handle_S_UpdateDurability(std::shared_ptr<Session>& session, const S_UpdateDurability& pkt) {};
	virtual void Handle_S_PlaySound(std::shared_ptr<Session> session, S_PlaySound& pkt) {};

public:
	// 멤버 변수 set
	std::shared_ptr<CMyPlayer>				GetMyPlayer() const { return my_player; }
	void									SetPlayer(std::shared_ptr<CMyPlayer> _player) { my_player = _player; }
	void									SetCamera(std::shared_ptr<CCamera> _camera) { camera = _camera; }
	std::shared_ptr<CCamera>&				GetCamera() { return camera; }

	SCENE_TYPE								GetSceneType() const { return scene_type; }

	std::vector<std::shared_ptr<CObject>>&	GetObjects() { return objects; }
	std::unordered_map<uint64, size_t>&     GetIDIndex() { return id_To_Index; }

	void									SetLight(std::unique_ptr<CLightManager> _light) { light = std::move(_light); }
	
	std::shared_ptr<CObjectFactory>&		GetFactory() { return factory; };
protected:
	SCENE_TYPE								scene_type;

	std::shared_ptr<CMyPlayer>				my_player;			// 내 플레이어
	std::shared_ptr<CCamera>				camera;

	std::vector<std::shared_ptr<CObject>>	objects;			// 다른 플레이어 or 몬스터 or 오브젝트
	std::unordered_map<uint64, size_t>	    id_To_Index;

	std::unique_ptr<CLightManager> light;
	std::shared_ptr<CObjectFactory> factory;
	std::shared_ptr<CUIManager> ui_manager;

	// 4월 15일 추가. 
	std::vector<uint64>						player_slot_ids;

	ImGuiID last_hovered_id_ = 0;

	// for shadow
	BoundingSphere scene_bounds{};

	ComPtr<ID3D12Resource> buffer_color_resource{};
	ComPtr<ID3D12Resource> buffer_normal_resource{};
	D3D12_CPU_DESCRIPTOR_HANDLE buffer_color_rtv_handle{};
	D3D12_CPU_DESCRIPTOR_HANDLE buffer_normal_rtv_handle{};
};