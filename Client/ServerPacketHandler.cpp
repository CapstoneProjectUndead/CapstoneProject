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

		// 로그인 성공
		CImGuiManager::GetInstance().SetSignUpResult(true);

		// 로딩창 끄기
		CImGuiManager::GetInstance().SetSignupLoading(false);
		CImGuiManager::GetInstance().SetLoginLoading(false);
		CImGuiManager::GetInstance().CloseAllWindow();
	}
	else {
		printf("signup fail...! \n");
		CImGuiManager::GetInstance().SetSignUpResult(false);
		CImGuiManager::GetInstance().SetSignUpAlarm(true);

		// 로딩창 끄기
		CImGuiManager::GetInstance().SetSignupLoading(false);
		CImGuiManager::GetInstance().SetLoginLoading(false);
		CImGuiManager::GetInstance().CloseAllWindow();
	}

	return true;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{
	// Title Scene으로 시작
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene->GetSceneType() == SCENE_TYPE::TITLE);

	// 로딩창 끄기
	CImGuiManager::GetInstance().SetSignupLoading(false);
	CImGuiManager::GetInstance().SetLoginLoading(false);
	CImGuiManager::GetInstance().SetSignInResult(true);
	CImGuiManager::GetInstance().CloseAllWindow();

	// User 생성
	std::shared_ptr<CUser> user = std::make_shared<CUser>();

	// session, id 저장
	user->SetSession(session);
	user->SetUserID(pkt.user_id);

	// User Refcount 증가
	CAST_SS(session)->SetUser(user);

	return true;
}

bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt)
{
	if (pkt.success) {
		CAST_SS(session)->SetUser(nullptr);
		CImGuiManager::GetInstance().SetIsOnlie(false);
	}

	return true;
}

bool Handle_S_CREATEROOM(std::shared_ptr<Session> session, S_CreateRoom& pkt)
{
	CImGuiManager::GetInstance().SetRoomCreateLoading(false);

	CImGuiManager::GetInstance().CloseAllWindow();

	CImGuiManager::GetInstance().ReserveResetFocus();

	CSceneManager::GetInstance().ChangeScene(SCENE_TYPE::LOBBY);

	return true;
}

bool Handle_S_MYPLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	std::shared_ptr<CMyPlayer> myPlayer = std::make_shared<CMyPlayer>();
	myPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);
	myPlayer->SetSession(session);
	myPlayer->SetID(pkt.info.id);
	myPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));

	Material m{};
	m.albedo = XMFLOAT4(1.0f, 0.2f, 0.2f, 1.0f);
	m.glossiness = 0.0f;
	myPlayer->SetMaterial(m);

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
	otherPlayer->SetID(pkt.info.id);
	otherPlayer->SetPosition(XMFLOAT3(pkt.info.x, pkt.info.y, pkt.info.z));
	otherPlayer->SetState(pkt.info.state);

	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	scene->EnterScene(otherPlayer, otherPlayer->GetID());

	return true;
}

bool Handle_S_PLAYERLIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();

	S_PLAYER_LIST::PlayerList userList = pkt.GetPlayerList();

	for (int i = 0; i < pkt.player_count; ++i) {

		// 다른 유저 생성
		std::shared_ptr<CPlayer> otherPlayer = std::make_shared<CPlayer>();
		otherPlayer->Initialize(GET_DEVICE, GET_CMD_LIST);

		// 다른 유저 ID 부여
		otherPlayer->SetID(userList[i].info.id);

		// 다른 유저 위치 부여
		otherPlayer->SetPosition(XMFLOAT3(userList[i].info.x, userList[i].info.y, userList[i].info.z));

		// 다른 유저 상태 부여
		otherPlayer->SetState(userList[i].info.state);

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
				if (player->GetID() == pkt.info.id)
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
	if (myPlayer != nullptr && myPlayer->GetID() == pkt.info.id) {
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
		auto it = indexMap.find(pkt.info.id);
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
			ObjectInfo info;
			info.yaw = pkt.info.yaw;
			info.pitch = pkt.info.pitch;
			info.roll = pkt.info.roll;
			otherPlayer->SetDestInfo(info);
		}

		OpponentFrameHistory state{};
		state.player_id = pkt.info.id;
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
