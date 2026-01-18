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
	S_RemovePlayer removePkt;
	removePkt.info.id = player->GetID();
	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_RemovePlayer>(removePkt);

	// 해당 플레이어의 퇴장을 다른 유저들에게 알려준다.
	if (nullptr != player) {
		for (int i = 0; i < (UINT)SCENE_TYPE::END; ++i) {
			CScene* scene = CSceneManager::GetInstance().GetScenes()[i].get();
			if (scene != nullptr) {
				auto it = scene->GetPlayers().find(player->GetID());
				if (it != scene->GetPlayers().end()) {
					for (auto& pl : scene->GetPlayers()) {
						if (pl.second->GetID() == player->GetID()) continue;
						pl.second->GetSession()->DoSend(sendBuffer);
					}
				}

				// 모든 Scene의 오브젝트 map을 순회하면서 해당 플레이어가 있다면 삭제
				scene->GetPlayers().erase(player->GetID());
			}
		}
	}
}

void CClientSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CClientPacketHandler::HandlePacket(session, buf, pktSize);
}
