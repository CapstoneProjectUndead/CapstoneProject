#include "pch.h"
#include "ClientPacketHandler.h"
#include "ClientSession.h"
#include "TimeManager.h"
#include "SceneManager.h"
#include "LobbyScene.h"
#include "Player.h"
#include "TitleScene.h"
#include "User.h"

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
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TITLE);
	
	activeScene->PushPacketJob(session
		, (CTitleScene*)activeScene
		, &CTitleScene::HandleLogIn
		, pkt);

	//CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	//assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);
	//
	//activeScene->PushPacketJob(session
	//	, (CLobbyScene*)activeScene
	//	, &CLobbyScene::EnterPlayer
	//	, pkt);

	return true;
}

bool Handle_C_LOGOUT(shared_ptr<Session> session, C_LOGOUT& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TITLE);

	activeScene->PushPacketJob(session
		, (CTitleScene*)activeScene
		, &CTitleScene::HandleLogOut
		, pkt);

	return true;
}

bool Handle_C_SIGNUP(shared_ptr<Session> session, C_SIGNUP& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::TITLE].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::TITLE);

	activeScene->PushPacketJob(session
		, (CTitleScene*)activeScene
		, &CTitleScene::HandleSignUp
		, pkt);

	return true;
}

bool Handle_C_PLAYERINPUT(shared_ptr<Session> session, C_Input& pkt)
{
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);

	activeScene->PushPacketJob(
		session,
		(CLobbyScene*)activeScene,
		&CLobbyScene::MovePlayer,
		pkt
	);

	return true;
}
