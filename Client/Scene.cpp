#include "stdafx.h"
#include "Camera.h"
#include "MyPlayer.h"
#include "Shader.h"
#include "Scene.h"
#include "ObjectFactory.h"

#include "GameFramework.h"
#include "ServerSession.h"
#include "User.h"
#include "NetworkClockManager.h"
#include "ImGuiManager.h"


CScene::CScene(SCENE_TYPE type)
	: scene_type(type)
{
	factory = std::make_shared<CObjectFactory>();
}

CScene::~CScene()
{

}

void CScene::Initialize()
{

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
	AnimateObjects(elapsedTime);

	if(camera)
		camera->Update(my_player->position, elapsedTime);
	if(light)
		light->Update(camera.get());
}

void CScene::Render(ID3D12GraphicsCommandList* commandList)
{
	if (camera)
		camera->SetViewportsAndScissorRects(commandList);

	for (const auto& shader : shaders) {
		shader.second->RenderBegin(commandList);

		if (camera)
			camera->UpdateShaderVariables(commandList);

		if (light)
			light->Render(commandList);

		for (const auto& obj : objects) {
			if (shader.first == obj->GetShader()) {
				shader.second->Render(commandList, obj.get());
			}
		}

		if (my_player) {
			if (shader.first == my_player->GetShader()) {
				shader.second->Render(commandList, my_player.get());
			}
		}

		shader.second->RenderEnd(commandList);
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

void CScene::EnterScene(std::shared_ptr<CObject> obj, UINT id)
{
	id_To_Index[id] = objects.size();
	objects.push_back(obj);
}

void CScene::LeaveScene(UINT id)
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

void CScene::Handle_S_Spawn_Player(std::shared_ptr<Session>& session, const S_SpawnPlayer& pkt)
{
	if (pkt.is_my_player) {
		{
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

			my_player->SetIsSingle(false);

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

		EnterScene(otherPlayer, otherPlayer->GetID());
	}
}

void CScene::Handle_S_PLAYER_LIST(S_PLAYER_LIST& pkt)
{
	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

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
		EnterScene(otherPlayer, otherPlayer->GetID());
	}
}

void CScene::Handle_S_Move_Player(std::shared_ptr<Session>& session, const S_Move& pkt)
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

			myPlayer->SetVelocity({ pkt.info.vx, pkt.info.vy, pkt.info.vz });
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
	LeaveScene(pkt.player_id);
}
