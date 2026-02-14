#include "pch.h"
// Server쪽 LobbyScene
#include "LobbyScene.h"
#include "ClientSession.h"
#include "Player.h"
#include "User.h"

#undef min
#undef max

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)


CLobbyScene::CLobbyScene()
    : CScene(SCENE_TYPE::LOBBY)
{
}

CLobbyScene::~CLobbyScene()
{
}

void CLobbyScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CLobbyScene::EnterPlayer(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// User 객체 생성
	shared_ptr<CUser> user;
	if (!CAST_CS(session)->GetUser()) {
		user = make_shared<CUser>();

		// ClientSession이 Plyaer를 참조. (refcount 증가)
		CAST_CS(session)->SetUser(user);
	}

	// Player 객체 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	user->SetPlayer(player);
	player->SetID(user->GetUserID());
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// Player 위치 지정 (임시)
	XMFLOAT3 pos{};
	pos.x = rand() % 4 + 1;
	pos.y = 0;
	pos.z = rand() % 3;
	player->SetPosition(pos);

	// Player도 ClientSession을 약한 참조 (refcount 증가 x)
	player->SetSession(session);

	// Player도 CUser를 약한 참조 (refcount 증가 x)
	player->SetUser(CAST_CS(session)->GetUser());

	// 지금 접속한 유저에게 로그인 허락 패킷 보냄
	{
		S_SpawnPlayer playerPkt;
		playerPkt.scene_type = SCENE_TYPE::LOBBY;
		playerPkt.info.player_id = player->GetID();
		playerPkt.info.x = player->GetPosition().x;
		playerPkt.info.y = player->GetPosition().y;
		playerPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_SpawnPlayer>(playerPkt);
		session->DoSend(sendBuffer);
	}

	// 지금 접속한 유저에게 다른 유저의 정보도 알려준다.
	// 여기서는 가변길이 패킷을 보낸다.
	{
		lock_guard<mutex> lg(players_lock);
		if (!players.empty()) {

			int32 cnt = players.size();
			int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

			// 예시, 꼭 LobbyScene일 필요는 없다.
			S_PLAYERLIST_WRITE pktWriter(SCENE_TYPE::LOBBY);

			S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

			int idx = 0;
			for (auto& pl : players) {
				if (pl.second->GetID() == player->GetID())
					continue;

				userList[idx++] = { NetPlayerInfo{pl.second->GetID(), pl.second->GetPosition().x, pl.second->GetPosition().y,
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
		addPkt.info.player_id = player->GetID();
		addPkt.scene_type = SCENE_TYPE::LOBBY;
		addPkt.info.x = player->GetPosition().x;
		addPkt.info.y = player->GetPosition().y;
		addPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_AddPlayer>(addPkt);
		BroadCast(sendBuffer, player->GetID());
	}
}

void CLobbyScene::EnterLobby(shared_ptr<CUser> user)
{
	// Player 객체 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	player->SetID(user->GetUserID());
	user->SetPlayer(player);

	// Player 위치 지정 (임시)
	XMFLOAT3 pos{};
	pos.x = rand() % 4 + 1;
	pos.y = 0;
	pos.z = rand() % 3;
	player->SetPosition(pos);

	// Player도 ClientSession을 약한 참조 (refcount 증가 x)
	player->SetSession(user->GetSession());

	// Player도 CUser를 약한 참조 (refcount 증가 x)
	player->SetUser(user);

	// Player가 속한 방ID
	player->SetRoomID(user->GetRoomID());

	// Player가 속한 Scene 설정
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);


	// 유저에게 입장 허락 패킷과 방에 있는 다른 유저의 정보를 알려준다..
	// 여기서는 가변길이 패킷을 보낸다.
	{
		// 먼저 그 방에 LobbyScene에 있는 유저들 정보를 보내준다.
		lock_guard<mutex> lg(players_lock);
		if (!players.empty()) {

			int32 cnt = players.size();
			int32 pktSize = sizeof(S_PLAYER_LIST) + sizeof(S_PLAYER_LIST::Player) * cnt;

			S_PLAYERLIST_WRITE pktWriter(SCENE_TYPE::LOBBY);

			S_PLAYERLIST_WRITE::UserList userList = pktWriter.ReserveUserList(players.size());

			int idx = 0;
			for (auto& pl : players) {
				if (pl.second->GetID() == player->GetID())
					continue;

				userList[idx++] = { NetPlayerInfo{pl.second->GetID(), pl.second->GetPosition().x, pl.second->GetPosition().y,
				pl.second->GetPosition().z} };
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			if (user->GetSession())
				user->GetSession()->DoSend(sendBuffer);
		}
	}
	
	// S_Enter_Room 패킷
	// 입장 허락.
	{
		S_EnterRoom enterPkt;
		enterPkt.success = true;
		enterPkt.room_id = user->GetRoomID();
		enterPkt.scene_type = player->GetCurrentSceneType();
		auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}

	// 유저 Scene에 입장
	EnterScene(player);

	// 다른 유저에게 지금 접속한 유저의 정보를 알려준다.
	{
		S_AddPlayer addPkt;
		addPkt.scene_type = SCENE_TYPE::LOBBY;
		addPkt.info.player_id = player->GetID();
		addPkt.info.x = player->GetPosition().x;
		addPkt.info.y = player->GetPosition().y;
		addPkt.info.z = player->GetPosition().z;

		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer<S_AddPlayer>(addPkt);
		BroadCast(sendBuffer, player->GetID());
	}
}

// 서버 권위 방식
void CLobbyScene::MovePlayer(shared_ptr<Session> session, const C_Input& pkt)
{
	auto it = players.find(pkt.info.player_id);
	if (it == players.end()) 
		return;

	auto mover = it->second; // 실제 움직인 플레이어

	if (pkt.seq_num <= mover->GetLastSequence())
		return;

	// 회전은 클라 권위 방식이기 때문에, 클라에서 받은 회전값을 적용한다.
	mover->SetYaw(pkt.info.yaw);
	mover->SetPitch(pkt.info.pitch);

	// 플레이어가 누른 입력과 시퀀스 넘버를 입력 큐에 저장
	InputData input{ pkt.info.w, pkt.info.a, pkt.info.s, pkt.info.d };
	PendingInput pInput{ input, pkt.seq_num };
	mover->PushInput(pInput);
}