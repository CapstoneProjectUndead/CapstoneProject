#include "pch.h"
#include "ClientPacketHandler.h"
#include <VarialbePacketWriter.h>
#include "ClientSession.h"
#include "SceneManager.h"
#include "TestScene.h"
#include "Player.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)

bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len)
{
	cout << "정의 되지 않은 패킷 ID 입니다!" << endl;
	assert(nullptr);
	return false;
}

bool Handle_C_LOGIN(std::shared_ptr<Session> session, C_LOGIN& pkt)
{
	// Player 객체 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// Player 위치 지정 (임시)
	Vec3 pos{};
	pos.x = rand() % 4 + 1;
	pos.y = rand() % 4 + 1;
	pos.z = 0;
	player->SetPosition(pos);

	// ClientSession이 Plyaer를 참조. (refcount 증가)
	CAST_CS(session)->SetPlayer(player);

	// Player도 ClientSession을 약한 참조 (refcount 증가 x)
	player->SetSession(session);

	// 지금 접속한 유저에게 로그인 허락 패킷 보냄
	{
		S_SpawnPlayer playerPkt;
		playerPkt.info.id = player->GetID();
		playerPkt.info.x = player->GetPosition().x;
		playerPkt.info.y = player->GetPosition().y;
		playerPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_SpawnPlayer>(playerPkt);
		session->DoSend(sendBuffer);
	}

	CScene* activeScene = CSceneManager::GetInstance().GetActiveScene();

	// 지금 접속한 유저에게 다른 유저의 정보도 알려준다.
	{
		if (activeScene) {
			const auto players = activeScene->GetPlayers();
			
			if (!players.empty()) {

				int32 cnt = activeScene->GetPlayers().size();
				int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

				S_PLAYERLIST_WRITE pktWriter;

				S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(activeScene->GetPlayers().size());

				int idx = 0;
				for (auto& pl : activeScene->GetPlayers()) {
					if (pl.second->GetID() == player->GetID())
						continue;

					userList[idx++] = { ObjectInfo{pl.second->GetID(), pl.second->GetPosition().x, pl.second->GetPosition().y,
					pl.second->GetPosition().z} };
				}

				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				session->DoSend(sendBuffer);
			}
		}
	}

	// 유저 Scene에 입장
	activeScene->EnterScene(player);

	// 다른 유저에게 지금 접속한 유저의 정보를 알려준다.
	{
		S_AddPlayer addPkt;
		addPkt.info.id = player->GetID();
		addPkt.info.x = player->GetPosition().x;
		addPkt.info.y = player->GetPosition().y;
		addPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_AddPlayer>(addPkt);
		activeScene->BroadCast(sendBuffer, player->GetID());
	}

	return true;
}

bool Handle_C_MOVE(std::shared_ptr<Session> session, C_Move& pkt)
{
	CScene* pScene =CSceneManager::GetInstance().GetActiveScene();

	CAST_CS(session)->GetPlayer()->SetPosition(pkt.info.x, pkt.info.y, pkt.info.z);

	S_Move movePkt;
	movePkt.info.id = pkt.info.id;
	movePkt.info.x = pkt.info.x;
	movePkt.info.y = pkt.info.y;
	movePkt.info.z = pkt.info.z;

	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);
	pScene->BroadCast(sendBuffer, pkt.info.id);

	return true;
}
