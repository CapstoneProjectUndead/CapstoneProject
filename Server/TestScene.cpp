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
	if (it == players.end()) return;

	auto mover = it->second; // 실제 움직인 플레이어

	// 1. 이동 처리 (클라 deltaTime 사용)
	float serverDt = std::min(pkt.deltaTime, 0.1f);

	InputData input{};
	input.w = pkt.info.w;
	input.a = pkt.info.a;
	input.s = pkt.info.s;
	input.d = pkt.info.d;

	mover->PredictMove(input, serverDt);
	mover->last_processed_seq = pkt.seq_num;
	mover->SetState(pkt.info.state);

	// 2. 움직인 플레이어와 다른 유저들에게 위치 통보
	{
		S_Move movePkt;

		movePkt.last_seq_num = pkt.seq_num;
		movePkt.info.id = mover->GetID(); // "움직인 사람"의 ID
		movePkt.info.x = mover->GetPosition().x;
		movePkt.info.y = mover->GetPosition().y;
		movePkt.info.z = mover->GetPosition().z;
		movePkt.info.yaw = pkt.info.yaw;
		movePkt.info.pitch = pkt.info.pitch;

		// 현재 입력 상태 (애니메이션용)
		movePkt.info.w = pkt.info.w;
		movePkt.info.a = pkt.info.a;
		movePkt.info.s = pkt.info.s;
		movePkt.info.d = pkt.info.d;

		movePkt.info.state = mover->state;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Move>(movePkt);
		session->DoSend(sendBuffer);

		// [중요] 시퀀스 번호는 오직 '움직인 본인'에게만 의미가 있음
		// 받는 사람이 mover일 때만 시퀀스를 넣어주고, 나머지에겐 0을 보냅니다.
		movePkt.last_seq_num = 0;

		// 움직인 플레이어를 제외한 나머지 유저에게 전달.
		BroadCast(sendBuffer, mover->GetID());
	}
}