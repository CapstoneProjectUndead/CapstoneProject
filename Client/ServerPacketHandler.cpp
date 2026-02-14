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
	// 회원가입은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_SignRes(session, pkt);

	return true;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{
	// 로그인은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_Login(session, pkt);

	return true;
}

bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt)
{
	// 로그아웃은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_Logout(session, pkt);

	return true;
}

bool Handle_S_CREATE_ROOM(std::shared_ptr<Session> session, S_CreateRoom& pkt)
{
	// 방 생성도 Title Scene에서 처리하는게 맞다.
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_CreateRoom(session, pkt);

	return true;
}

bool Handle_S_ENTER_ROOM(std::shared_ptr<Session> session, S_EnterRoom& pkt)
{
	// 서버에서 방 입장 허락이 왔는데, 입장 씬이 Title이면 안된다. 
	// Title Scene은 Room이 아니기 때문이다.  Title -> [Room](Lobby Scene, Game Scene) 
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 하지만 방 입장은 Title Scene에서 처리한다.
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_EnterRoom(session, pkt);

	return true;
}

bool Handle_S_ROOMLIST(std::shared_ptr<Session> session, S_Room_List& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_RoomList(session, pkt);

	return true;
}

bool Handle_S_SPAWN_PLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt)
{
	// 서버에서 플레이어를 spawn 하라고 했는데, Title Scene이면 안된다.
	// Title 씬은 플레이 씬이 아니다. 
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 서버가 통보한 Scene에 플레이어를 추가한다.
	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(scene);
	scene->Handle_S_Spawn_Player(session, pkt);

	return true;
}

bool Handle_S_PLAYER_LIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 서버가 통보한 Scene에 플레이어들을 추가한다.
	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(scene);
	scene->Handle_S_PLAYER_LIST(pkt);

	return true;
}

bool Handle_S_REMOVE_PLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt)
{
#ifdef SCENE_TEST
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
#else
	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();

	for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
		CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
		if (scene != nullptr) {
			for (auto& player : scene->GetObjects()) {
				if (player->GetID() == pkt.info.player_id)
					scene->LeaveScene(player->GetID());
			}
		}
	}
#endif

	return true;
}

bool Handle_S_MOVE(std::shared_ptr<Session> session, S_Move& pkt)
{
	// Title Scene에는 플레이어가 없다.
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(scene);
	scene->Handle_S_Move_Player(session, pkt);

	return true;
}
