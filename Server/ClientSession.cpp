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
		if (player) {
			if (player->GetRoomID() != -1) {
				auto& roomManager = CRoomManager::GetInstance();

				lock_guard<mutex> lg(roomManager.GetMutex());
				auto room = roomManager.FindRoom(player->GetRoomID());
				if (room) {
					// 플레이어가 있는 룸에서 플레이어가 속한 씬에서 플레이어를 제거
					room->GetScenes()[(UINT)player->GetCurrentSceneType()]->LeaveScene(player->GetID());

					// 해당 방의 씬들에 유저들이 하나도 없다면 방 삭제!
					if (room->SearchPlayersAllScene()) {
						roomManager.DestroyRoomNoLock(room->GetRoomID());
					}
				}
			}
		}

	}

	user = nullptr;

#endif

}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
