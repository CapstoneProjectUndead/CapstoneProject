#include "stdafx.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "LobbyScene.h"
#include "Player.h"
#include "MyPlayer.h"
#include "GeometryLoader.h"
#include "Camera.h"
#include "Shader.h"
#include "Movement.h"
#include "NetworkClockManager.h"
#include "NetworkManager.h"
#include "ImGuiManager.h"
#include "User.h"
#include "TitleScene.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_SS(session) static_pointer_cast<CServerSession>(session)

bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len)
{
	//std::cout << "정의 되지 않은 패킷 ID 입니다!" << std::endl;
	assert(nullptr);
	return false;
}

bool Handle_S_PING(std::shared_ptr<Session> session, S_Ping& pkt)
{
	C_Pong pongPkt;
	pongPkt.server_send_time = pkt.server_send_time; // 서버가 보낸 시간 그대로 복사
	auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Pong>(pongPkt);
	session->DoSend(sendBuffer);

	return true;
}

bool Handle_S_PONG(std::shared_ptr<Session> session, S_Pong& pkt)
{
	float now = CNetworkClockManager::GetInstance().GetClientNow();
	CNetworkClockManager::GetInstance().UpdateClockSync(pkt.clientTime, pkt.serverTime, now);
	return true;
}

bool Handle_S_SIGNRES(std::shared_ptr<Session> session, S_SIGN_RES& pkt)
{
	if (pkt.success) {
		printf("signup success! \n");

		// 가입 성공
		CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "가입 성공!");

		// 로딩창 끄기
		CSceneManager::GetInstance().GetTitleScene()->StopLoading();
	}
	else {
		printf("signup fail...! \n");

		// 가입 실패
		CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(false, "가입 실패..!");

		// 로딩창 끄기
		CSceneManager::GetInstance().GetTitleScene()->StopLoading();
	}

	return true;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{
	// Title Scene으로 시작
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene->GetSceneType() == SCENE_TYPE::TITLE);

	// 로그인 성공
	if (pkt.success) {
		CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "로그인 성공!");

		// 로딩창 끄기
		CSceneManager::GetInstance().GetTitleScene()->StopLoading();

		// User 생성
		std::shared_ptr<CUser> user = std::make_shared<CUser>();

		// session, id 저장 (약한 참조)
		user->SetSession(session);
		user->SetUserID(pkt.user_id);

		// User Refcount 증가
		CAST_SS(session)->SetUser(user);
	}
	// 로그인 실패
	else {
		CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(false, "로그인 실패!");

		// 로딩창 끄기
		CSceneManager::GetInstance().GetTitleScene()->StopLoading();
	}

	return true;
}

bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt)
{
	if (pkt.success) {
		CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "로그아웃 성공!");
		CSceneManager::GetInstance().GetTitleScene()->StopLoading();
		CAST_SS(session)->SetUser(nullptr);
	}

	return true;
}

bool Handle_S_CREATEROOM(std::shared_ptr<Session> session, S_CreateRoom& pkt)
{
	RoomInfo info{ pkt.room_info.room_id, pkt.room_info.room_name, pkt.room_info.current_player_count, pkt.room_info.is_game_start };
	CSceneManager::GetInstance().GetTitleScene()->GetRooms().insert({ info.room_id, info });

	CSceneManager::GetInstance().GetTitleScene()->SetIsEnter(true);

	CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "방 생성 완료!");

	CAST_SS(session)->GetUser()->SetRoomID(info.room_id);

	// 로딩창 끄기
	CSceneManager::GetInstance().GetTitleScene()->StopLoading();

	return true;
}

bool Handle_S_ENTERROOM(std::shared_ptr<Session> session, S_EnterRoom& pkt)
{
	CSceneManager::GetInstance().GetTitleScene()->SetIsEnter(true);

	CSceneManager::GetInstance().GetTitleScene()->ShowResultPopup(true, "방 입장 완료!");

	CAST_SS(session)->GetUser()->SetRoomID(pkt.room_id);

	// 로딩창 끄기
	CSceneManager::GetInstance().GetTitleScene()->StopLoading();

	return true;
}

bool Handle_S_ROOMLIST(std::shared_ptr<Session> session, S_Room_List& pkt)
{
	S_Room_List::RoomList userList = pkt.GetRoomList();

	uint32* newRoom = nullptr;
	if (pkt.room_count > 0)
		newRoom = new uint32[pkt.room_count];

	for (int i = 0; i < pkt.room_count; ++i) {
		newRoom[i] = userList[i].room_info.room_id;

		RoomInfo info{userList[i].room_info.room_id, userList[i].room_info.room_name
			, userList[i].room_info.current_player_count, userList[i].room_info.is_game_start};

		CSceneManager::GetInstance().GetTitleScene()->GetRooms().insert({ info.room_id, info });
	}
	
	// 프로그램을 끄지않고 로그아웃하고 로그인 했을 때,
	// 기존에는 있었는데 없어진 방 체크
	auto& rooms = CSceneManager::GetInstance().GetTitleScene()->GetRooms();
	if (pkt.room_count == 0) {
		rooms.clear();
	}
	else {
		for (auto it = rooms.begin(); it != rooms.end(); ) {
			int id = it->first;

			bool found = false;
			for (int i = 0; i < pkt.room_count; ++i) {
				if (newRoom[i] == id) {
					found = true;
					break;
				}
			}

			if (!found) {
				it = rooms.erase(it); // 삭제 + 다음 iterator 반환
			}
			else {
				++it;
			}
		}
	}

	if (newRoom != nullptr)
		delete[] newRoom;

	return true;
}

bool Handle_S_MYPLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	std::shared_ptr<CMyPlayer> myPlayer = std::make_shared<CMyPlayer>();
	myPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);
	myPlayer->SetSession(session);
	myPlayer->SetID(pkt.info.player_id);
	myPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
	myPlayer->SetCurrentSceneType(pkt.scene_type);

	{
		std::shared_ptr<CShader> shader = std::make_unique<CShader>();
		shader->CreateShader(GET_DEVICE);
		scene->GetShaders().emplace("static", std::move(shader));
	}

	{
		std::shared_ptr<CShader> shader = std::make_unique<CSkinningShader>();
		shader->CreateShader(GET_DEVICE);
		scene->GetShaders().emplace("skinning", std::move(shader));
	}

	// 카메라 객체 생성
	RECT client_rect;
	GetClientRect(ghWnd, &client_rect);
	float width{ float(client_rect.right - client_rect.left) };
	float height{ float(client_rect.bottom - client_rect.top) };

	std::shared_ptr<CCamera> camera = std::make_shared<CCamera>();
	camera->Initialize(GET_DEVICE, GET_CMD_LIST);
	camera->SetTarget(myPlayer.get());

	camera->CreateConstantBuffers(GET_DEVICE, GET_CMD_LIST);

	scene->SetPlayer(myPlayer);
	scene->SetCamera(camera);

	// light 생성
	std::unique_ptr<CLightManager>light = std::make_unique<CLightManager>();
	light->Initialize(GET_DEVICE, GET_CMD_LIST);

	scene->SetLight(std::move(light));

	return true;
}

bool Handle_S_ADDPLAYER(std::shared_ptr<Session> session, S_AddPlayer& pkt)
{
	std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>();
	otherPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);
	otherPlayer->SetID(pkt.info.player_id);
	otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
	otherPlayer->SetState(pkt.info.state);

	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	scene->EnterScene(otherPlayer, otherPlayer->GetID());

	return true;
}

bool Handle_S_PLAYERLIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();

	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저 생성
		std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>();
		otherPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);

		// 다른 유저 ID 부여
		otherPlayer->SetID(userList[i].info.player_id);

		// 다른 유저 위치 부여
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// 다른 유저 상태 부여
		otherPlayer->SetState(userList[i].info.state);

		// 다른 유저가 속한 씬 설정
		otherPlayer->SetCurrentSceneType(pkt.scene_type);

		// Active Scene에 다른 유저 입장
		scene->EnterScene(otherPlayer, otherPlayer->GetID());
	}

	return true;
}

bool Handle_S_REMOVEPLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
		CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
		if (scene != nullptr) {
			for (auto& player : scene->GetObjects()) {
				if (player->GetID() == pkt.info.player_id)
					scene->LeaveScene(player->GetID());
			}
		}
	}

	return true;
}

bool Handle_S_MOVE(std::shared_ptr<Session> session, S_Move& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	auto& vec = scene->GetObjects();
	auto& indexMap = scene->GetIDIndex();
	std::shared_ptr<CMyPlayer> myPlayer = scene->GetMyPlayer();

	// 내 플레이어이면, 내 플레이어 보정용 함수 호출
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.player_id) {
		myPlayer->SetVelocity(pkt.info.vx, pkt.info.vy, pkt.info.vz);

		// 서버가 처리한 시퀀스 넘버를 받아야한다.
		myPlayer->ReconcileFromServer(pkt.last_seq_num, XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

		// 여기서 S_Move 패킷의 지터값 측정
		float now = CNetworkClockManager::GetInstance().GetClientNow();
		CNetworkManager::GetInstance().GetJitterMeasurer()->OnPacketArrival(now);
	}
	// 다른 플레이어일 경우
	else {
		// 해당 ID가 존재하는 플레이어인지 확인
		auto it = indexMap.find(pkt.info.player_id);
		if (it == indexMap.end())
			return false;

		uint64 idx = it->second;
		if (idx >= vec.size())
			return false;

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

	return true;
}
