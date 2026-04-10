#include "pch.h"
#include "ClientSession.h"
#include "SceneManager.h"
#include "User.h"
#include "Player.h"
#include "LobbyScene.h"
#include "TitleScene.h"
#include "RoomManager.h"


CClientSession::CClientSession()
{
}

CClientSession::~CClientSession()
{
}

void CClientSession::OnConnected()
{
	cout << "ClientSession 접속 성공!" << endl;
}

void CClientSession::OnDisconnected()
{

#ifdef LOBBY_SCENE_TEST
	if (user) {
		auto player = user->GetPlayer();
		if (player) {
			CScene* scene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
			if (scene) {
				PktDummy dummyPkt;
				dummyPkt.value = player->GetID();

				scene->PushPacketJob(GetSessionRef()
					, (CScene*)scene
					, &CScene::Handle_C_Player_Leave
					, dummyPkt);
			}
		}
	}
#else
	// User를 ClientSession에서도 관리하고 (ref 증가)
	// Title 씬에서도 관리하고 있다. (ref 증가)

	if (user) {
		// Title 씬에서 User 참조 끊기 (ref 감소)
		CSceneManager::GetInstance().GetTitleScene()->LeaveUser(user->GetUserID());

		auto player = user->GetPlayer();
		if (player) {
			C_LeaveRoom leavePkt;
			leavePkt.user_id = user->GetUserID();
			CRoomManager::GetInstance().LeaveAndCleanupRoom(GetSessionRef(), leavePkt);
		}

		// ClientSession에서 User 참조 끊기 (ref 감소)
		// 여기서 User 소멸. (메모리 누수x)
		user = nullptr;
	}

#endif

}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
