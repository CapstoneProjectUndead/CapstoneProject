#include "pch.h"
#include "ClientSession.h"
#include "SceneManager.h"
#include "User.h"
#include "Player.h"
#include "LobbyScene.h"
#include "TitleScene.h"


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
		auto player = user->GetPlayer();
		for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {

			auto& rooms = CSceneManager::GetInstance().GetRooms();
			auto iter = rooms.find(user->GetRoomID());
			auto& room = iter->second;
			room->GetScenes()[(UINT)player->GetCurrentSceneType()]->LeaveScene(player->GetID());
		}

		CSceneManager::GetInstance().GetTitleScene()->LeaveUser(user->GetUserID());
	}
#endif 

	user = nullptr;
}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
