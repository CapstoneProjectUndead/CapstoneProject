#include "pch.h"
#include "ClientSession.h"
#include "SceneManager.h"
#include "Player.h"

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
	if (nullptr != player) {
		for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
			if(CSceneManager::GetInstance().GetScenes()[i] != nullptr)
				CSceneManager::GetInstance().GetScenes()[i]->GetPlayers().erase(player->GetID());
		}
	}
}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
