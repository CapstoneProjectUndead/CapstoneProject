#include "pch.h"
// Server쪽 CustomScene
#include "CustomScene.h"
#include "LobbyScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"


CCustomScene::CCustomScene()
	: CScene(SCENE_TYPE::CUSTOMS)
{

}

CCustomScene::CCustomScene(uint32 roomId)
	: CScene(SCENE_TYPE::CUSTOMS, roomId)
{
}

CCustomScene::~CCustomScene()
{
}

void CCustomScene::Start()
{
}

void CCustomScene::Update(float elapsedTime)
{

}

void CCustomScene::C_Handle_Enter_CustomScene(shared_ptr<Session> session, const C_EnterRoom& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	assert(user);

	uint32 roomId = pkt.room_id;
	auto room = user->GetRoom();
	assert(room);

	// 플레이어 생성
	shared_ptr<CPlayer> player = CObject::CreatePlayer();

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어의 (플레이어 ID = 유저 ID)
	player->SetID(user->GetUserID());

	// 지금 플레이어가 속한 방ID
	player->SetRoomID(roomId);

	// 지금 플레이어가 속한 방
	player->SetRoom(room);

	// Player가 속한 Scene 설정
	player->SetCurrentSceneType(SCENE_TYPE::LOBBY);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 방 인원 수 증가
	room->PlayerEnter();

	// Custom Scene에는 별도로 EnterScene 하지 않도록 결정.

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
		lobbyScene->EnterScene(player);

		// 유저에게 커스터마이징 완료되었고,
		// Lobby Scene으로 씬 전환하라고 알려주기
		S_CustomSelect pkt;
		auto sendBuffer = MAKE_SEND_BUFFER(pkt);
		session->DoSend(sendBuffer);
	}
}
