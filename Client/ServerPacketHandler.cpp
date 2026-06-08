#include "stdafx.h"
#include "ServerPacketHandler.h"
#include "ServerSession.h"
#include "GameFramework.h"
#include "SceneManager.h"
#include "LobbyScene.h"
#include "Player.h"
#include "MyPlayer.h"
#include "GeometryLoader.h"
#include "Camera.h"
#include "Shader.h"
#include "Movement.h"
#include "NetworkClockManager.h"
#include "NetworkManager.h"
#include "ImGuiManager.h"
#include "User.h"
#include "TitleScene.h"
#include "CustomScene.h"
#include "GameScene.h"


PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

#define CAST_SS(session) static_pointer_cast<CServerSession>(session)

bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len)
{
	//std::cout << "정의 되지 않은 패킷 ID 입니다!" << std::endl;
	assert(nullptr);
	return false;
}

bool Handle_S_PING(std::shared_ptr<Session> session, S_Ping& pkt)
{
	C_Pong pongPkt;
	pongPkt.server_send_time = pkt.server_send_time; // 서버가 보낸 시간 그대로 복사
	auto sendBuffer = CServerPacketHandler::MakeSendBuffer<C_Pong>(pongPkt);
	session->DoSend(sendBuffer);

	return true;
}

bool Handle_S_PONG(std::shared_ptr<Session> session, S_Pong& pkt)
{
	float now = CNetworkClockManager::GetInstance().GetClientNow();
	CNetworkClockManager::GetInstance().UpdateClockSync(pkt.clientTime, pkt.serverTime, now);
	return true;
}

bool Handle_S_SIGNRES(std::shared_ptr<Session> session, S_SIGN_RES& pkt)
{
	// 회원가입은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_SignRes(session, pkt);

	return true;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{
	// 로그인은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_Login(session, pkt);

	return true;
}

bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt)
{
	// 로그아웃은 Title Scene에서 처리
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_Logout(session, pkt);

	return true;
}

bool Handle_S_ENTER_ROOM(std::shared_ptr<Session> session, S_EnterRoom& pkt)
{
	// 서버에서 방 입장 허락이 왔는데, 입장 씬이 Title이면 안된다. 
	// Title Scene은 Room이 아니기 때문이다.  Title -> [Room](Custom Scene, Lobby Scene, Game Scene) 
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 하지만 방 입장은 Title Scene에서 처리한다.
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_EnterRoom(session, pkt);

	return true;
}

bool Handle_S_ROOMLIST(std::shared_ptr<Session> session, S_Room_List& pkt)
{
	CTitleScene* titleScene = CSceneManager::GetInstance().GetTitleScene();
	assert(titleScene);
	titleScene->Handle_S_RoomList(session, pkt);

	return true;
}

bool Handle_S_SPAWN_PLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt)
{
	// 서버에서 플레이어를 spawn 하라고 했는데, Title Scene이면 안된다.
	// Title 씬은 플레이 씬이 아니다. 
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 서버가 통보한 Scene에 플레이어를 추가한다.
	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);
	targetScene->Handle_S_Spawn_Player(session, pkt);

	return true;
}

bool Handle_S_PLAYER_LIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt)
{
	if (pkt.scene_type == SCENE_TYPE::TITLE)
		assert(nullptr);

	// 서버가 통보한 Scene에 플레이어들을 추가한다.
	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);
	targetScene->Handle_S_PLAYER_LIST(pkt);

	return true;
}

bool Handle_S_REMOVE_PLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt)
{
#ifdef SCENE_TEST
	CScene* scene = CSceneManager::GetInstance().GetActiveScene();
	assert(scene);
	scene->Handle_S_Remove_Player(session, pkt);
#else
	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);
	targetScene->Handle_S_Remove_Player(session, pkt);
#endif

	return true;
}

bool Handle_S_PLAYER_MOVE(std::shared_ptr<Session> session, S_PlayerMove& pkt)
{
	// Title 씬과 Custom 씬에는 플레이어가 없다.
	if (pkt.scene_type == SCENE_TYPE::TITLE || pkt.scene_type == SCENE_TYPE::CUSTOMS)
		return true;

	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);
	targetScene->Handle_S_Move_Player(session, pkt);

	return true;
}

bool Handle_S_CUSTOM_SELECT(std::shared_ptr<Session> session, S_CustomSelect& pkt)
{
	CCustomScene* customScene = (CCustomScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get();
	assert(customScene && customScene->GetSceneType() == SCENE_TYPE::CUSTOMS);
	customScene->Handle_S_Custom_Select(session, pkt);

	return true;
}

bool Handle_S_SPAWN_MONSTER(std::shared_ptr<Session> session, S_SpawnMonster& pkt)
{
	// Title 씬과 Custom 씬에는 몬스터가 존재할 수 없다.
	if (pkt.scene_type == SCENE_TYPE::TITLE || pkt.scene_type == SCENE_TYPE::CUSTOMS)
		assert(nullptr);

	SCENE_TYPE sceneType = pkt.scene_type;
	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)sceneType].get();
	assert(targetScene && targetScene->GetSceneType() == sceneType);
	targetScene->Handle_S_Spawn_Monster(session, pkt);

	return true;
}

bool Handle_S_DESPAWN_MONSTER(std::shared_ptr<Session> session, S_DeSpawnMonster& pkt)
{
	// Title 씬과 Custom 씬에는 몬스터가 존재할 수 없다.
	if (pkt.scene_type == SCENE_TYPE::TITLE || pkt.scene_type == SCENE_TYPE::CUSTOMS)
		assert(nullptr);

	SCENE_TYPE sceneType = pkt.scene_type;
	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)sceneType].get();
	assert(targetScene && targetScene->GetSceneType() == sceneType);
	targetScene->Handle_S_DeSpawn_Monster(session, pkt);

	return true;
}

bool Handle_S_MONSTER_MOVE(std::shared_ptr<Session> session, S_MonsterMove& pkt)
{
	// Title 씬과 Custom 씬에는 플레이어가 없다.
	if (pkt.scene_type == SCENE_TYPE::TITLE || pkt.scene_type == SCENE_TYPE::CUSTOMS)
		assert(nullptr);

	CScene* targetScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.scene_type].get();
	assert(targetScene);
	targetScene->Handle_S_Move_Monster(session, pkt);

	return true;
}

bool Handle_S_SCENE_CHANGE(std::shared_ptr<Session> session, S_SceneChange& pkt)
{
	CScene* currentScene = CSceneManager::GetInstance().GetScenes()[(UINT)pkt.current_scene].get();
	assert(currentScene);

	// 유저가 현재 속한 Scene이 서버쪽 기록과 다르면 assert!
	auto player = CAST_SS(session)->GetUser()->GetMyPlayer();
	assert(player->GetCurrentSceneType() == pkt.current_scene);

	currentScene->Handle_S_Scene_Change(session, pkt);

	return true;
}

bool Handle_S_MAP_START(std::shared_ptr<Session> session, S_MapStart& pkt)
{
	CLobbyScene* lobbyScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);
	lobbyScene->Handle_S_MapStart(session, pkt);

	return true;
}

bool Handle_S_MAP_DATA(std::shared_ptr<Session> session, S_MapData& pkt)
{
	CGameScene* gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	assert(gameScene);
	gameScene->Handle_S_MapData(session, pkt);

	return true;
}

bool Handle_S_MAP_END(std::shared_ptr<Session> session, S_MapEnd& pkt)
{
	CGameScene* gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	assert(gameScene);
	gameScene->Handle_S_MapEnd(session, pkt);

	return true;
}

bool Handle_S_SPAWN_ITEM(std::shared_ptr<Session> session, S_SpawnItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_SpawnItem(session, pkt);

	return true;
}

bool Handle_S_SPAWN_ITEM_LIST(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_SpawnItemList(session, pkt);

	return true;
}

bool Handle_S_DESPAWN_ITEM(std::shared_ptr<Session> session, S_DeSpawnItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_DeSpawnItem(session, pkt);

	return true;
}

bool Handle_S_ADD_ITEM(std::shared_ptr<Session> session, S_AddItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_AddItem(session, pkt);

	return true;
}

bool Handle_S_ADD_ITEM_LIST(std::shared_ptr<Session> session, S_AddItemList& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get();
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_AddItemList(session, pkt);

	return true;
}

bool Handle_S_REMOVE_ITEM(std::shared_ptr<Session> session, S_RemoveItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_RemoveItem(session, pkt);

	return true;
}

bool Handle_S_READY(std::shared_ptr<Session> session, S_Ready& pkt)
{
	CLobbyScene* lobbyScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	assert(lobbyScene);
	lobbyScene->Handle_S_Ready(session, pkt); 
	return true;
}

bool Handle_S_EQUIP_ITEM(std::shared_ptr<Session> session, S_EquipItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_EquipItem(session, pkt);

	return true;
}

bool Handle_S_USE_ITEM(std::shared_ptr<Session> session, S_UseItem& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_UseItem(session, pkt);

	return true;
}

bool Handle_S_MINEABLE_LIST(std::shared_ptr<Session> session, S_MineableList& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_MineableList(session, pkt);

	return true;
}

bool Handle_S_DESTROY_MINEABLE(std::shared_ptr<Session> session, S_DestroyMineable& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_DestroyMineable(session, pkt);

	return true;
}

bool Handle_S_UPDATE_DURABILITY(std::shared_ptr<Session> session, S_UpdateDurability& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_UpdateDurability(session, pkt);

	return true;
}

bool Handle_S_PLAY_SOUND(std::shared_ptr<Session> session, S_PlaySound& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_PlaySound(session, pkt);

	return true;
}

bool Handle_S_CHOLD_FAIL(std::shared_ptr<Session> session, S_CHoldFail& pkt)
{
	CGameScene* gameScene = nullptr;

	gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();

	if (!gameScene)
		return true;

	gameScene->Handle_S_CHoldFail(session, pkt);

	return true;
}

bool Handle_S_RETURN_ZONE_ACTIVE(std::shared_ptr<Session> session, S_ReturnZoneActive& pkt)
{
	if (pkt.scene_type != SCENE_TYPE::GAME)
		return true;

	CGameScene* gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	if (!gameScene)
		return true;

	gameScene->Handle_S_ReturnZoneActive(session, pkt);

	return true;
}

bool Handle_S_PLAYER_RETURNED(std::shared_ptr<Session> session, S_PlayerReturned& pkt)
{
	if (pkt.scene_type != SCENE_TYPE::GAME)
		return true;

	CGameScene* gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	if (!gameScene)
		return true;

	gameScene->Handle_S_PlayerReturned(session, pkt);

	return true;
}

bool Handle_S_GAME_SETTLEMENT(std::shared_ptr<Session> session, S_GameSettlement& pkt)
{
	if (pkt.scene_type != SCENE_TYPE::GAME)
		return true;

	CGameScene* gameScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
	if (!gameScene)
		return true;

	gameScene->Handle_S_GameSettlement(session, pkt);

	return true;
}

bool Handle_S_UPDATE_COIN(std::shared_ptr<Session> session, S_UpdateCoin& pkt)
{
	CScene* targetScene = nullptr;

	switch (pkt.scene_type)
	{
	case SCENE_TYPE::NONE:
		break;
	case SCENE_TYPE::TITLE:
		break;
	case SCENE_TYPE::CUSTOMS:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::CUSTOMS].get();
		break;
	case SCENE_TYPE::LOBBY:
		targetScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
		break;
	case SCENE_TYPE::GAME:
		targetScene = (CGameScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::GAME].get();
		break;
	case SCENE_TYPE::UI:
		break;
	default:
		break;
	}

	if (!targetScene)
		return true;

	targetScene->Handle_S_UpdateCoin(session, pkt);

	return true;
}

bool Handle_S_REFRESH_STORE(std::shared_ptr<Session> session, S_RefreshStore& pkt)
{
	CLobbyScene* lobbyScene = (CLobbyScene*)CSceneManager::GetInstance().GetScenes()[(UINT)SCENE_TYPE::LOBBY].get();
	if (!lobbyScene)
		return true;

	lobbyScene->Handle_S_RefreshStore(session, pkt);

	return true;
}
