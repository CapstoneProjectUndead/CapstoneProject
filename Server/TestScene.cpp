#include "pch.h"
// Server쪽 TestScene
#include "TestScene.h"
#include "ClientSession.h"
#include "Player.h"

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

				userList[idx++] = { PackObjectInfo{pl.second->GetID(), pl.second->GetPosition().x, pl.second->GetPosition().y,
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

// 클라 권위 방식
void CTestScene::MovePlayer(shared_ptr<Session> session, const C_Move& pkt)
{
	CAST_CS(session)->GetPlayer()->SetPosition(pkt.info.x, pkt.info.y, pkt.info.z);

	S_Move movePkt;
	movePkt.info.id = pkt.info.id;
	movePkt.info.state = pkt.info.state;
	movePkt.info.x = pkt.info.x;
	movePkt.info.y = pkt.info.y;
	movePkt.info.z = pkt.info.z;
	movePkt.info.yaw = pkt.info.yaw;
	movePkt.info.pitch = pkt.info.pitch;
	movePkt.info.roll = pkt.info.roll;

	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);
	BroadCast(sendBuffer, pkt.info.id);
}

// 서버 권위 방식
void CTestScene::MovePlayer(shared_ptr<Session> session, const C_Input& pkt)
{
    auto it = players.find(pkt.info.id);
    if (it == players.end())
        return;

    auto player = players[it->first];

	player->last_processed_seq = pkt.seq_num;
	player->current_input = pkt.info.input;

	//player->Move();

	// 일단은 여기서 클라들에게 전송해준다.
	// 나중에 반드시 바깥에서 전송하는걸로 바꿔야 한다. 
	for (auto& pl : players) {
		S_Move movePkt;
		movePkt.info.id = pl.second->GetID();
		movePkt.info.x = pl.second->GetPosition().x;

		// ... (좌표 및 회전 값 대입) ...
		movePkt.info.x = player->GetPosition().x;
		movePkt.info.y = player->GetPosition().y;
		movePkt.info.z = player->GetPosition().z;
		movePkt.info.yaw = player->GetYaw();
		movePkt.info.pitch = player->GetPitch();

		// [핵심] 서버가 "너의 n번 입력을 처리한 결과가 이거야"라고 알려줌
		movePkt.last_seq_num = player->last_processed_seq;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);

		BroadCast(sendBuffer);
	}
}