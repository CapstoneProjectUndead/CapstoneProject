#include "pch.h"
#include "ClientPacketHandler.h"
#include "ClientSession.h"
#include "TimeManager.h"
#include "SceneManager.h"
#include "LobbyScene.h"
#include "Player.h"
#include "TitleScene.h"
#include "User.h"
#include "RoomManager.h"


PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)

bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len)
{
	cout << "정의 되지 않은 패킷 ID 입니다!" << endl;
	assert(nullptr);
	return false;
}

bool Handle_C_PING(shared_ptr<Session> session, C_Ping& pkt)
{
	S_Pong pongPkt;
	pongPkt.clientTime = pkt.clientTime;
	pongPkt.serverTime = g_server_total_time;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Pong>(pongPkt);
	session->DoSend(sendBuffer);

	return true;
}

bool Handle_C_PONG(shared_ptr<Session> session, C_Pong& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	if (!user)
		return true;

	// 현재 시간 - 패킷에 담겨 돌아온 '보냈던 시간'
	float rtt = g_server_total_time - pkt.server_send_time;

	// 너무 튀는 값 방지를 위한 보정 (Moving Average)
	user->GetPlayer()->UpdatePing(rtt);

	return true;
}

bool Handle_C_LOGIN(shared_ptr<Session> session, C_LOGIN& pkt)
{
#ifdef SCENE_TEST
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);
	
	activeScene->PushPacketJob(session
		, (CLobbyScene*)activeScene
		, &CLobbyScene::EnterPlayer
		, pkt);
#else
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene->GetSceneType() == SCENE_TYPE::TITLE);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::HandleLogIn
		, pkt);

	const auto& users = CSceneManager::GetInstance().GetTitleScene()->GetUsers();
	for (auto& user : users) {
		auto session = user.second->GetSession();
		if (session) {

		}
	}

#endif

	return true;
}

bool Handle_C_LOGOUT(shared_ptr<Session> session, C_LOGOUT& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene->GetSceneType() == SCENE_TYPE::TITLE);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::HandleLogOut
		, pkt);

	return true;
}

bool Handle_C_SIGNUP(shared_ptr<Session> session, C_SIGNUP& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene->GetSceneType() == SCENE_TYPE::TITLE);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::HandleSignUp
		, pkt);

	return true;
}

bool Handle_C_CREATEROOM(shared_ptr<Session> session, C_CreateRoom& pkt)
{
	auto& roomManger = CRoomManager::GetInstance();
	uint32 roomID = roomManger.CreateRoom(pkt.room_name, CAST_CS(session)->GetUser());

	// 유저에게 방 생성을 허락.
	S_CreateRoom createRoomPkt;
	createRoomPkt.room_id = roomID;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_CreateRoom>(createRoomPkt);
	session->DoSend(sendBuffer);

	return true;
}

bool Handle_C_PLAYERINPUT(shared_ptr<Session> session, C_Input& pkt)
{
#ifdef SCENE_TEST
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);

	activeScene->PushPacketJob(
		session,
		(CLobbyScene*)activeScene,
		&CLobbyScene::MovePlayer,
		pkt
	);
#else

#endif

	return true;
}
