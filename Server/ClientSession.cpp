#include "pch.h"
#include "ClientSession.h"
#include "SceneManager.h"
#include "Player.h"
#include "TestScene.h"


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
			CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
			if (scene != nullptr) {
				scene->LeaveScene(player->GetID());
			}
		}
	}
}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
