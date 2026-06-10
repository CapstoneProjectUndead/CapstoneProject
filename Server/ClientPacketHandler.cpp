#include "pch.h"
#include "ClientPacketHandler.h"
#include "ClientSession.h"
#include "TimeManager.h"
#include "SceneManager.h"
#include "LobbyScene.h"
#include "GameScene.h"
#include "Player.h"
#include "TitleScene.h"
#include "User.h"
#include "RoomManager.h"
#include "CustomScene.h"


PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)

bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len)
{
	cout << "정의 되지 않은 패킷 ID 입니다!" << endl;
	assert(nullptr);
	return false;
}

bool Handle_C_PING(shared_ptr<Session> session, C_Ping& pkt)
{
	S_Pong pongPkt;
	pongPkt.clientTime = pkt.clientTime;
	pongPkt.serverTime = g_server_total_time;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_Pong>(pongPkt);
	session->DoSend(sendBuffer);

	return true;
}

bool Handle_C_PONG(shared_ptr<Session> session, C_Pong& pkt)
{
	auto user = CAST_CS(session)->GetUser();
	if (!user)
		return true;

	// 현재 시간 - 패킷에 담겨 돌아온 '보냈던 시간'
	float rtt = g_server_total_time - pkt.server_send_time;

	// 너무 튀는 값 방지를 위한 보정 (Moving Average)
	user->GetPlayer()->UpdatePing(rtt);

	return true;
}

bool Handle_C_LOGIN(shared_ptr<Session> session, C_LOGIN& pkt)
{
#ifdef LOBBY_SCENE_TEST
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);
	
	activeScene->PushPacketJob(session
		, (CLobbyScene*)activeScene
		, &CLobbyScene::C_Enter_Player
		, pkt);
#else
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::Handle_C_LogIn
		, pkt);

#endif

	return true;
}

bool Handle_C_LOGOUT(shared_ptr<Session> session, C_LOGOUT& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene->GetSceneType() == SCENE_TYPE::TITLE);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::Handle_C_LogOut
		, pkt);

	return true;
}

bool Handle_C_SIGNUP(shared_ptr<Session> session, C_SIGNUP& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene->GetSceneType() == SCENE_TYPE::TITLE);

	titleScene->PushPacketJob(session
		, (CTitleScene*)titleScene
		, &CTitleScene::Handle_C_SignUp
		, pkt);

	return true;
}

bool Handle_C_CREATE_ROOM(shared_ptr<Session> session, C_CreateRoom& pkt)
{
	CRoomManager::GetInstance().CreateRoom(session, pkt);
	return true;
}

bool Handle_C_ENTER_ROOM(shared_ptr<Session> session, C_EnterRoom& pkt)
{
	CRoomManager::GetInstance().EnterRoom(session, pkt);
	return true;
}

bool Handle_C_LEAVE_ROOM(shared_ptr<Session> session, C_LeaveRoom& pkt)
{
	CRoomManager::GetInstance().LeaveAndCleanupRoom(session, pkt);
	auto user = CAST_CS(session)->GetUser();
	user->SetPlayer(nullptr);
	user->SetRoom(nullptr);
	return true;
}

bool Handle_C_UPDATE_ROOM(shared_ptr<Session> session, C_UpdateRoom& pkt)
{
	CRoomManager::GetInstance().SendRoomList(session);
	return true;
}

bool Handle_C_PLAYER_INPUT(shared_ptr<Session> session, C_Input& pkt)
{
#ifdef LOBBY_SCENE_TEST
	CScene* activeScene = CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(activeScene->GetSceneType() == SCENE_TYPE::LOBBY);

	activeScene->PushPacketJob(
		session,
		(CScene*)activeScene,
		&CScene::Handle_C_Player_Input,
		pkt
	);
#else
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room);

	CScene* currentScene = room->GetScenes()[(UINT)pkt.scene_type].get();
	assert(currentScene);

	currentScene->PushPacketJob(
		session,
		(CScene*)currentScene,
		&CScene::Handle_C_Player_Input,
		pkt
	);

#endif

	return true;
}

bool Handle_C_CUSTOM_SELECT(std::shared_ptr<Session> session, C_CustomSelect& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room);
	CCustomScene* customScene = dynamic_cast<CCustomScene*>(room->GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get());
	assert(customScene);

	customScene->PushPacketJob(
		session,
		(CCustomScene*)customScene,
		&CCustomScene::C_Handle_Custom_Select,
		pkt
	);

	return true;
}

bool Handle_C_SCENE_CHANGE(std::shared_ptr<Session> session, C_SceneChange& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* currentScene = room->GetScenes()[(UINT)pkt.current_scene].get();
	assert(currentScene);

	// 유저가 현재 속한 Scene이 서버쪽 기록과 다르면 assert!
	auto player = CAST_CS(session)->GetUser()->GetPlayer();
	assert(player->GetCurrentSceneType() == pkt.current_scene);

	currentScene->PushPacketJob(session,
		(CScene*)currentScene,
		&CScene::Handle_C_Scene_Change,
		pkt);

	return true;
}

bool Handle_C_READY(std::shared_ptr<Session> session, C_Ready& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CLobbyScene* lobbyScene = (CLobbyScene*)room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_Ready,
		pkt);

	return true;
}

bool Handle_C_PICKUP_ITEM(std::shared_ptr<Session> session, C_PickupItem& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* targetScene = room->GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);

	targetScene->PushPacketJob(session,
		(CScene*)targetScene,
		&CScene::Handle_C_Pickup_Item,
		pkt);

	return true;
}

bool Handle_C_DROP_ITEM(std::shared_ptr<Session> session, C_DropItem& pkt)
{
	if (pkt.scene_type != SCENE_TYPE::GAME)
		return true;

	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* targetScene = room->GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);

	targetScene->PushPacketJob(session,
		(CScene*)targetScene,
		&CScene::Handle_C_Drop_Item,
		pkt);

	return true;
}

bool Handle_C_EQUIP_ITEM(std::shared_ptr<Session> session, C_EquipItem& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* targetScene = room->GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);

	targetScene->PushPacketJob(session,
		(CScene*)targetScene,
		&CScene::Handle_C_Equip_Item,
		pkt);

	return true;
}

bool Handle_C_USE_ITEM(std::shared_ptr<Session> session, C_UseItem& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* targetScene = room->GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);

	targetScene->PushPacketJob(session,
		(CScene*)targetScene,
		&CScene::Handle_C_Use_Item,
		pkt);

	return true;
}

bool Handle_C_GIVE_UP_RESCUE(std::shared_ptr<Session> session, C_GiveUpRescue& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* gameScene = room->GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	assert(gameScene);

	gameScene->PushPacketJob(session,
		(CGameScene*)gameScene,
		&CGameScene::Handle_C_GiveUpRescue,
		pkt);

	return true;
}

bool Handle_C_SHOP_STATE(std::shared_ptr<Session> session, C_ShopState& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* lobbyScene = room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_ShopState,
		pkt);

	return true;
}

bool Handle_C_BUY_ITEM(std::shared_ptr<Session> session, C_BuyItem& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* lobbyScene = room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_BuyItem,
		pkt);

	return true;
}

bool Handle_C_SPEND_GOLD(std::shared_ptr<Session> session, C_SpendGold& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* lobbyScene = room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_SpendGold,
		pkt);

	return true;
}

bool Handle_C_REFRESH_STORE(std::shared_ptr<Session> session, C_RefreshStore& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* lobbyScene = room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_RefreshStore,
		pkt);

	return true;
}

bool Handle_C_EXTENSE_INVENTORY(std::shared_ptr<Session> session, C_ExtenseInventory& pkt)
{
	auto room = CAST_CS(session)->GetUser()->GetRoom();
	assert(room->IsActive());
	CScene* lobbyScene = room->GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);

	lobbyScene->PushPacketJob(session,
		(CLobbyScene*)lobbyScene,
		&CLobbyScene::Handle_C_ExtenseInventory,
		pkt);

	return true;
}
