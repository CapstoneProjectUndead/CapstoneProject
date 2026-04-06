#include "stdafx.h"
#include "Camera.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "Scene.h"
#include "ObjectFactory.h"
#include "PhysicsManager.h"
#include "Collider.h"

#include "GameFramework.h"
#include "ServerSession.h"
#include "User.h"
#include "NetworkClockManager.h"
#include "ImGuiManager.h"
#include "Monster.h"

#include "UIComponent.h"
#include "Renderers.h"
#include "SceneManager.h"

CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
{
	factory = std::make_shared<CObjectFactory>();
	ui_manager = std::make_shared<CUIManager>();
}

CScene::~CScene()
{

}

void CScene::Initialize()
{
	factory->GetMaterial(CSceneManager::GetInstance().GetShaders()["ui"]->GetHeapManager(), "white");	// 인덱스 0에 생성하기 위해 먼저 생성
}

void CScene::ReleaseUploadBuffers()
{
	for (const auto& obj : objects) {
		obj->ReleaseUploadBuffer();
	}
}

void CScene::AnimateObjects(float elapsedTime)
{
	if (my_player) {
		my_player->Update(elapsedTime);
	}

	for (const auto& obj : objects) {
		obj->Update(elapsedTime);
	}
}

void CScene::Update(float elapsedTime)
{
	CPhysicsManager::GetInstance().Update(elapsedTime);
	AnimateObjects(elapsedTime);

	if(camera)
		camera->Update(my_player->position, elapsedTime);
	if(light)
		light->Update(camera.get());

	ui_manager->Update(elapsedTime);
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera) camera->SetViewportsAndScissorRects(commandList);

	auto& shaders = CSceneManager::GetInstance().GetShaders();
	auto& renderers = CSceneManager::GetInstance().GetRanderers();
	// Collect Phase
	// 3D 객체들 수집
	for (const auto& obj : objects) {
		auto it = renderers.find(obj->GetShader());
		if (it != renderers.end()) {
			obj->OnCollect(it->second.get());
		}
	}

	// 플레이어 수집
	if (my_player) {
		auto it = renderers.find(my_player->GetShader());
		if (it != renderers.end()) {
			my_player->OnCollect(it->second.get());
		}
	}

	// UI 매니저 수집
	ui_manager->Collect(renderers);

	// Draw Phase
	for (const auto& [name, pShader] : shaders) {
		pShader->RenderBegin(commandList);

		if (name == "billboard") {
			camera->UpdateShaderVariablesBillBoard(commandList);
		}
		else if (name == "ui") {
			camera->UpdateShaderVariables(commandList, true);
		}
		else {
			camera->UpdateShaderVariables(commandList, false);
		}

		bool useLight = (name != "ui" && name != "billboard");
		if (light && useLight) {
			light->Render(commandList);
		}

		// 인스턴싱 드로우
		auto it = renderers.find(name);
		if (it != renderers.end()) {
			it->second->Render(commandList);
			if (name == "ui") {
				renderers["text"]->Render(commandList);
			}
		}

		pShader->RenderEnd(commandList);
	}
}

void CScene::Exit()
{
	CPhysicsManager::GetInstance().ClearCollider();

	RemoveAllMonsters();

	last_input_state = !last_input_state;
}

void CScene::Enter()
{
	for (const auto& obj : objects) {
		CColliderComponent* col = obj->GetComponent<CColliderComponent>();
		if (col) {
			CPhysicsManager::GetInstance().SetCollider(col);
		}
	}
}

void CScene::DrawUI_Final()
{
	ManageIME();  // 공통 로직 (IME/포커스)
	DrawUI();     // 자식이 구현할 구체적인 UI 로직
}

void CScene::ManageIME()
{
	// =======================
	// [IME 및 포커스 관리 로직]
	// =======================
	bool currentInputState = IsUIInputEnabled();
	HWND hwnd = ghWnd;

	// 상태 변경 시에만 IME 제어
	if (currentInputState != last_input_state || CImGuiManager::need_reset_focus) {

		if (currentInputState) {
			CImGuiManager::EnableIME(hwnd);
		}
		else {
			CImGuiManager::DisableIME(hwnd);
		}

		if (CImGuiManager::need_reset_focus) {
			ImGuiContext& g = *GImGui;
			g.ActiveId = 0;
			ImGui::SetWindowFocus(nullptr);
			ImGui::GetIO().InputQueueCharacters.resize(0);
			ImGui::GetIO().ClearInputKeys();
			CImGuiManager::ResetIMEState(hwnd);

			CImGuiManager::need_reset_focus = false;
		}

		last_input_state = currentInputState;
	}
}

void CScene::AddObject(std::shared_ptr<CObject> obj, UINT id)
{
	id_To_Index[id] = objects.size();
	objects.push_back(obj);
}

void CScene::RemoveObject(UINT id)
{
	auto iter = id_To_Index.find(id);
	if (iter == id_To_Index.end())
		return;

	UINT idx = id_To_Index[id];
	UINT last = objects.size() - 1;

	std::swap(objects[idx], objects[last]);
	id_To_Index[objects[idx]->GetID()] = idx;

	objects.pop_back();
	id_To_Index.erase(id);
}

void CScene::RemoveAllMonsters()
{
	std::vector<UINT> eraseIds;

	for (const auto& [id, idx] : id_To_Index) {
		if (id >= 1001)
			eraseIds.push_back(id);
	}

	for (UINT id : eraseIds) {
		RemoveObject(id);
	}
}

void CScene::Handle_S_Spawn_Player(std::shared_ptr<Session>& session, const S_SpawnPlayer& pkt)
{
	auto shaders = CSceneManager::GetInstance().GetShaders();

	if (pkt.is_my_player) {
		{
			// 싱글 모드가 아닌 멀티 모드로 전환
			g_is_single = false;

			CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
			my_player = factory->CreateMyPlayer(skinningHeapManager);

			// 세션 저장
			my_player->SetSession(session);
			// 아이디 저장
			my_player->SetID(pkt.info.player_id);
			// 방 ID 저장
			my_player->SetRoomID(pkt.room_id);
			// 서버가 알려준 위치에 spawn
			my_player->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
			// 플레이어가 속한 Scene
			my_player->SetCurrentSceneType(pkt.scene_type);

			if (SERVER_SESSION) {
				auto user = SERVER_SESSION->GetUser();
				if (user) {
					// user는 player를 강한 참조
					SERVER_SESSION->GetUser()->SetMyPlayer(my_player);
					SERVER_SESSION->GetUser()->SetRoomID(pkt.room_id);

					// player는 user를 약한 참조
					my_player->SetUser(SERVER_SESSION->GetUser());
				}
			}
		}
	}
	else {
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		std::shared_ptr<CPlayer> otherPlayer = factory->CreatePlayer(skinningHeapManager);
		otherPlayer->SetID(pkt.info.player_id);
		otherPlayer->SetRoomID(pkt.room_id);
		otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
		otherPlayer->SetState(pkt.info.state);
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		otherPlayer->ChangeModelSet(pkt.info.body_type);
		otherPlayer->ChangeEyes(pkt.info.eyes_type);
		otherPlayer->ChangeMouth(pkt.info.mouth_type);

		AddObject(otherPlayer, otherPlayer->GetID());
		player_slot_ids.push_back(otherPlayer->GetID());
	}
}

void CScene::Handle_S_PLAYER_LIST(S_PLAYER_LIST& pkt)
{
	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	auto shaders = CSceneManager::GetInstance().GetShaders();
	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저의 Player 생성
		CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };
		std::shared_ptr<CPlayer> otherPlayer = factory->CreatePlayer(skinningHeapManager);

		// 다른 유저의 Player ID 부여
		otherPlayer->SetID(userList[i].info.player_id);

		// 다른 유저의 Player 위치 부여
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// 다른 유저의 Player 상태 부여
		otherPlayer->SetState(userList[i].info.state);

		// 다른 유저의 Player 속한 씬 설정
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		// 굳이이긴 하나, 그래도 그냥 set 한다.
		otherPlayer->SetRoomID(pkt.room_id);

		// 다른 플레이어의 캐릭터 커스터마이즈
		otherPlayer->ChangeModelSet(userList[i].info.body_type);
		otherPlayer->ChangeEyes(userList[i].info.eyes_type);
		otherPlayer->ChangeMouth(userList[i].info.mouth_type);

		// Active Scene에 다른 유저 입장
		AddObject(otherPlayer, otherPlayer->GetID());

		player_slot_ids.push_back(otherPlayer->GetID());
	}
}

void CScene::Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_PlayerMove& pkt)
{
	auto& vec = GetObjects();
	auto& indexMap = GetIDIndex();
	std::shared_ptr<CMyPlayer> myPlayer = GetMyPlayer();

	// 내 플레이어이면, 내 플레이어 보정용 함수 호출
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.player_id) {

		myPlayer->SetServerVelocity(XMFLOAT3{pkt.info.vx, pkt.info.vy, pkt.info.vz});

		// 예측 이동을 없애고
		// 아래의 코드가 추가되었다.
		{
			PlayerInfo info;
			info.x = pkt.info.x;
			info.y = pkt.info.y;
			info.z = pkt.info.z;
			myPlayer->SetDestInfo(info);
		}

		// 서버가 처리한 시퀀스 넘버를 받아야한다.
		//myPlayer->ReconcileFromServer(pkt.last_seq_num, XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

		// 여기서 S_Move 패킷의 지터값 측정
		float now = CNetworkClockManager::GetInstance().GetClientNow();
		CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(now);
	}
	// 다른 플레이어일 경우
	else {
		// 해당 ID가 존재하는 플레이어인지 확인
		auto it = indexMap.find(pkt.info.player_id);
		if (it == indexMap.end())
			return;

		uint64 idx = it->second;
		if (idx >= vec.size())
			return;

		auto otherPlayer = std::static_pointer_cast<CPlayer>(vec[idx]);
		otherPlayer->SetYaw(pkt.info.yaw);
		otherPlayer->SetPitch(pkt.info.pitch);
		otherPlayer->SetState(pkt.info.state);

		// 회전을 위해 남겨둠
		{
			PlayerInfo info;
			info.yaw = pkt.info.yaw;
			info.pitch = pkt.info.pitch;
			info.roll = pkt.info.roll;
			otherPlayer->SetDestInfo(info);
		}

		// 상대 캐릭터는 서버 타임스탬프 기반 엔티티 보간 
		OpponentFrameHistory state{};
		state.player_id = pkt.info.player_id;
		state.state = pkt.info.state;
		state.position = XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z);
		state.server_timestamp = pkt.timestamp;

#ifdef GENERATE_LAG
		// 렉 시뮬레이터 작동
		CNetworkManager::GetInstance().OnRecvOpponentPos(state);
#else
		otherPlayer->RecordOpponentFrameHistory(state);

		// 여기서 S_Move 패킷의 지터값 측정
		float now = CNetworkClockManager::GetInstance().GetClientNow();
		CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(now);
#endif
	}
}

void CScene::Handle_S_Remove_Player(std::shared_ptr<Session>& session, const S_RemovePlayer& pkt)
{
	RemoveObject(pkt.player_id);
}

void CScene::Handle_S_Spawn_Monster(std::shared_ptr<Session>& session, const S_SpawnMonster& pkt)
{
	auto shaders = CSceneManager::GetInstance().GetShaders();
	
	CDescriptorHeapManager* skinningHeapManager{ shaders["skinning"]->GetHeapManager() };

	MON_TYPE type = pkt.info.monster_type;
	NetMonsterInfo info = pkt.info;

	auto monster = factory->CreateMonster(skinningHeapManager, type, scene_type);
	monster->SetID(info.monster_id);
	monster->SetPosition(0.f, 0.1f, -1.5f);
	monster->SetOriginPos({ 0.f, 0.1f, -1.5f });
	AddObject(monster, info.monster_id);
}

void CScene::Handle_S_Move_Monster(std::shared_ptr<Session>& session, const S_MonsterMove& pkt)
{
	auto& vec = GetObjects();
	auto& indexMap = GetIDIndex();

	// 해당 ID가 존재하는 플레이어인지 확인
	auto it = indexMap.find(pkt.info.monster_id);
	if (it == indexMap.end())
		return;

	uint64 idx = it->second;
	if (idx >= vec.size())
		return;

	auto monster = std::static_pointer_cast<CMonster>(vec[idx]);

	// 상대 캐릭터는 서버 타임스탬프 기반 엔티티 보간 
	MonsterFrameHistory state{};
	state.monster_id = pkt.info.monster_id;
	state.AI_state = pkt.info.AI_state;
	state.position = XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z);
	state.yaw = pkt.info.yaw;
	state.pitch = pkt.info.pitch;
	state.server_timestamp = pkt.timestamp;

	monster->RecordMonsterFrameHistory(state);
}

void CScene::Handle_S_Scene_Change(std::shared_ptr<Session>& session, const S_SceneChange& pkt)
{
	CSceneManager::GetInstance().ChangeScene(pkt.target_scene);
}
