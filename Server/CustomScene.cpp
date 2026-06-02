#include "pch.h"
// Server쪽 CustomScene
#include "CustomScene.h"
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Movement.h"


CCustomScene::CCustomScene(uint32 roomId)
	: CScene(SCENE_TYPE::CUSTOMS, roomId)
{
}

CCustomScene::~CCustomScene()
{
}

void CCustomScene::Initialize()
{
	CScene::Initialize();
}

void CCustomScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CCustomScene::OnSceneActivate()
{
	CScene::OnSceneActivate();
}

void CCustomScene::OnSceneDeactivate()
{
	CScene::OnSceneDeactivate();
}

void CCustomScene::EnterScene(shared_ptr<CPlayer> player)
{
	// 커스텀씬은 개인 공간이라 다른 플레이어와 안 보이게 broadcast 생략.
	// players 컨테이너 등록과 current_scene_type 갱신만 처리.
	players[player->GetID()] = player;
	player->SetCurrentSceneType(scene_type);
}

void CCustomScene::C_Handle_Enter_CustomScene(shared_ptr<Session> session, const C_EnterRoom& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	auto room = user->GetRoom();
	assert(room);

	// 3월 19일 추가
	// 방에 있던 기존 플레이어들이 사신에게 말을 걸어서 준비완료하면 GameScene으로 넘어간다.
	// 그 찰나에 새로운 유저가 들어온다면 입장을 막아야한다.
	if (room->GetIsGameStart()) {
		S_EnterRoom enterPkt;
		enterPkt.success = false;
		enterPkt.room_id = pkt.room_id;
		auto sendBuffer = MAKE_SEND_BUFFER(enterPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);

		return;
	}

	// Player 생성 (플레이어 ID = 유저 ID)
	shared_ptr<CPlayer> player = CServerObjectFactory::CreatePlayer(SCENE_TYPE::CUSTOMS, session, user, room, GetPhysicsManager());
	player->SetCoin(10000);

	// Custom Scene에는 별도로 EnterScene 하지 않도록 결정했는데
	// 5월 19일 기준, 이제 커스텀씬도 입장함
	EnterScene(player);

	// S_SpawnPlayer 패킷
	// 지금 방에 입장한 유저에게 플레이어 생성 허락
	{
		S_SpawnPlayer spawnPkt;
		spawnPkt.room_id = player->GetRoomID();
		spawnPkt.scene_type = SCENE_TYPE::CUSTOMS;
		spawnPkt.is_my_player = true;
		spawnPkt.info.player_id = player->GetID();
		spawnPkt.info.room_id = player->GetRoomID();
		spawnPkt.info.is_my_player = true;
		spawnPkt.info.x = 0;
		spawnPkt.info.y = 0;
		spawnPkt.info.z = 0;

		auto sendBuffer = MAKE_SEND_BUFFER(spawnPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
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

	// 6월 2일 추가
	{
		S_UpdateCoin coinPkt;
		coinPkt.player_id = player->GetID();
		coinPkt.coin = 10000;
		coinPkt.scene_type = scene_type;
		auto sendBuffer = MAKE_SEND_BUFFER(coinPkt);
		if (user->GetSession())
			user->GetSession()->DoSend(sendBuffer);
	}
}

void CCustomScene::C_Handle_Custom_Select(shared_ptr<Session> session, const C_CustomSelect& pkt)
{
	auto player = CAST_CS(session)->GetUser()->GetPlayer();
	assert(player);
	
	player->SetBodyType(pkt.body_type);
	player->SetEyesType(pkt.eye_type);
	player->SetMouthType(pkt.mouth_type);

	if (auto r = room.lock()) {
		CLobbyScene* lobbyScene = (CLobbyScene*)r->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		assert(lobbyScene && lobbyScene->GetSceneType() == SCENE_TYPE::LOBBY);

		// 플레이어를 Lobby Scene으로 이동
		// 유저 Scene에 입장
		// EnterScene 에서 유저들의 입장 정보들을 다 처리하도록 수정. (26. 2. 25)
		// (26. 5. 19) 아래 ChangeScene 함수 호출 x
		//ChangeScene(player, SCENE_TYPE::LOBBY);

		// 유저에게 커스터마이징 완료
		S_CustomSelect pkt;
		auto sendBuffer = MAKE_SEND_BUFFER(pkt);
		session->DoSend(sendBuffer);
	}
}
