#include "pch.h"
// Server쪽 TestScene
#include "TestScene.h"
#include "ClientSession.h"
#include "Player.h"

#undef min
#undef max

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)


CTestScene::CTestScene()
    : CScene(SCENE_TYPE::TEST)
{
}

CTestScene::~CTestScene()
{
}

void CTestScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CTestScene::EnterPlayer(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// Player 객체 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// Player 위치 지정 (임시)
	XMFLOAT3 pos{};
	pos.x = rand() % 4 + 1;
	pos.y = 0;
	pos.z = rand() % 3;
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

	// 지금 접속한 유저에게 다른 유저의 정보도 알려준다.
	{
		if (!players.empty()) {

			int32 cnt = players.size();
			int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

			S_PLAYERLIST_WRITE pktWriter;

			S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

			int idx = 0;
			for (auto& pl : players) {
				if (pl.second->GetID() == player->GetID())
					continue;

				userList[idx++] = { NetObjectInfo{pl.second->GetID(), pl.second->GetPosition().x, pl.second->GetPosition().y,
				pl.second->GetPosition().z} };
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			session->DoSend(sendBuffer);
		}
	}

	// 유저 Scene에 입장
	EnterScene(player);

	// 다른 유저에게 지금 접속한 유저의 정보를 알려준다.
	{
		S_AddPlayer addPkt;
		addPkt.info.id = player->GetID();
		addPkt.info.x = player->GetPosition().x;
		addPkt.info.y = player->GetPosition().y;
		addPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_AddPlayer>(addPkt);
		BroadCast(sendBuffer, player->GetID());
	}
}

// 서버 권위 방식
void CTestScene::MovePlayer(shared_ptr<Session> session, const C_Input& pkt)
{
	auto it = players.find(pkt.info.id);
	if (it == players.end()) 
		return;

	auto mover = it->second; // 실제 움직인 플레이어

	InputData input{ pkt.info.w, pkt.info.a, pkt.info.s, pkt.info.d };

	mover->SetLastSequence(pkt.seq_num);
	mover->SetClientDT(pkt.deltaTime);
	mover->SetInput(input);
	mover->SetState(pkt.info.state);
	
	// 회전 입력
	mover->SetYaw(pkt.info.yaw);
	mover->SetPitch(pkt.info.pitch);

}