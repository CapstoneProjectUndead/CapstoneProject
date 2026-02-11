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

#ifdef SCENE_TEST
	if (nullptr != user) {
		auto player = user->GetPlayer();
		for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
			CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
			if (scene != nullptr) {
				scene->LeaveScene(player->GetID());
			}
		}
	}
#else
	if (nullptr != user) {
		CSceneManager::GetInstance().GetTitleScene()->LeaveUser(user->GetUserID());

		auto player = user->GetPlayer();
		if (!player)
			return;

		if (player->GetRoomID() == -1)
			return;

		auto& rooms = CRoomManager::GetInstance().GetRooms();
		auto iter = rooms.find(player->GetRoomID());
		if (iter == rooms.end())
			return;

		auto& room = iter->second;
		room->GetScenes()[(UINT)player->GetCurrentSceneType()]->LeaveScene(player->GetID());

	}
#endif 

	user = nullptr;
}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
