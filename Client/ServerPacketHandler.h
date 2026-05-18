#pragma once
#include "Timer.h"
#include "NetworkManager.h"

using PacketHandlerFunc = std::function<bool(std::shared_ptr<Session>, char*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Packet ID
enum : uint16
{
	PKT_DUMMY = 0,
	PKT_S_PING,
	PKT_C_PONG,
	PKT_C_PING,
	PKT_S_PONG,
	PKT_C_SIGNUP,
	PKT_S_SIGNRES,
	PKT_C_LOGIN,
	PKT_C_LOGOUT,
	PKT_S_LOGIN,
	PKT_S_LOGOUT,
	PKT_C_CREATE_ROOM,
	PKT_C_UPDATE_ROOM,
	PKT_C_ENTER_ROOM,
	PKT_C_LEAVE_ROOM,
	PKT_C_ENTER_SCENE,
	PKT_S_ENTER_ROOM,
	PKT_S_LEAVE_ROOM,
	PKT_S_ENTER_SCENE,
	PKT_S_ROOM_LIST,
	PKT_S_SPAWN_PLAYER,
	PKT_S_PLAYER_LIST,
	PKT_S_REMOVE_PLAYER,

	PKT_C_PLAYER_INPUT,	// 서버 권위 방식 + 클라 예측 이동
	PKT_S_MOVE,

	PKT_C_CUSTOM_SELECT,
	PKT_S_CUSTOM_SELECT,
	PKT_C_SCENE_CHANGE,
	PKT_S_SCENE_CHANGE,
	PKT_S_SPAWN_MONSTER,
	PKT_S_DESPAWN_MONSTER,
	PKT_S_MONSTER_MOVE,

	PKT_S_MAP_START,
	PKT_S_MAP_DATA,
	PKT_S_MAP_END,

	PKT_C_READY,	// 로비씬에서 사신에게 준비 완료 버튼 누름
	PKT_S_READY,

	PKT_S_SPAWN_ITEM,
	PKT_S_SPAWN_ITEM_LIST,
	PKT_S_DESPAWN_ITEM,
	PKT_C_PICKUP_ITEM,
	PKT_S_ADD_ITEM,
	PKT_S_ADD_ITEM_LIST,
	PKT_S_REMOVE_ITEM,
	PKT_C_DROP_ITEM,
	PKT_C_EQUIP_ITEM,
	PKT_S_EQUIP_ITEM,
	PKT_C_USE_ITEM,
	PKT_S_USE_ITEM,

	PKT_S_MINEABLE_LIST,
	PKT_S_DESTROY_MINEABLE,
	PKT_S_UPDATE_DURABILITY,

	PKT_S_PLAY_SOUND,

	PKT_S_POSSESSION_RELEASE_FAIL,

	// 정산 시스템
	PKT_S_RETURN_ZONE_ACTIVE,
	PKT_S_PLAYER_RETURNED,
};

// Custom Handlers
bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len);
bool Handle_S_PING(std::shared_ptr<Session> session, S_Ping& pkt);
bool Handle_S_PONG(std::shared_ptr<Session> session, S_Pong& pkt);
bool Handle_S_SIGNRES(std::shared_ptr<Session> session, S_SIGN_RES& pkt);
bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt);
bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt);
bool Handle_S_ENTER_ROOM(std::shared_ptr<Session> session, S_EnterRoom& pkt);
bool Handle_S_ROOMLIST(std::shared_ptr<Session> session, S_Room_List& pkt);
bool Handle_S_SPAWN_PLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt);
bool Handle_S_PLAYER_LIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt);
bool Handle_S_REMOVE_PLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt);
bool Handle_S_PLAYER_MOVE(std::shared_ptr<Session> session, S_PlayerMove& pkt);
bool Handle_S_CUSTOM_SELECT(std::shared_ptr<Session> session, S_CustomSelect& pkt);
bool Handle_S_SPAWN_MONSTER(std::shared_ptr<Session> session, S_SpawnMonster& pkt);
bool Handle_S_DESPAWN_MONSTER(std::shared_ptr<Session> session, S_DeSpawnMonster& pkt);
bool Handle_S_MONSTER_MOVE(std::shared_ptr<Session> session, S_MonsterMove& pkt);
bool Handle_S_SCENE_CHANGE(std::shared_ptr<Session> session, S_SceneChange& pkt);
bool Handle_S_MAP_START(std::shared_ptr<Session> session, S_MapStart& pkt);
bool Handle_S_MAP_DATA(std::shared_ptr<Session> session, S_MapData& pkt);
bool Handle_S_MAP_END(std::shared_ptr<Session> session, S_MapEnd& pkt);
bool Handle_S_SPAWN_ITEM(std::shared_ptr<Session> session, S_SpawnItem& pkt);
bool Handle_S_SPAWN_ITEM_LIST(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt);
bool Handle_S_DESPAWN_ITEM(std::shared_ptr<Session> session, S_DeSpawnItem& pkt);
bool Handle_S_ADD_ITEM(std::shared_ptr<Session> session, S_AddItem& pkt);
bool Handle_S_ADD_ITEM_LIST(std::shared_ptr<Session> session, S_AddItemList& pkt);
bool Handle_S_REMOVE_ITEM(std::shared_ptr<Session> session, S_RemoveItem& pkt);
bool Handle_S_READY(std::shared_ptr<Session> session, S_Ready& pkt);
bool Handle_S_EQUIP_ITEM(std::shared_ptr<Session> session, S_EquipItem& pkt);
bool Handle_S_USE_ITEM(std::shared_ptr<Session> session, S_UseItem& pkt);
bool Handle_S_MINEABLE_LIST(std::shared_ptr<Session> session, S_MineableList& pkt);
bool Handle_S_DESTROY_MINEABLE(std::shared_ptr<Session> session, S_DestroyMineable& pkt);
bool Handle_S_UPDATE_DURABILITY(std::shared_ptr<Session> session, S_UpdateDurability& pkt);
bool Handle_S_PLAY_SOUND(std::shared_ptr<Session> session, S_PlaySound& pkt);
bool Handle_S_POSSESSION_RELEASE_FAIL(std::shared_ptr<Session> session, S_PossessionReleaseFail& pkt);
bool Handle_S_RETURN_ZONE_ACTIVE(std::shared_ptr<Session> session, S_ReturnZoneActive& pkt);
bool Handle_S_PLAYER_RETURNED(std::shared_ptr<Session> session, S_PlayerReturned& pkt);

class CServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		GPacketHandler[PKT_S_PING] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Ping>(Handle_S_PING, session, buffer, len); };
		GPacketHandler[PKT_S_PONG] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Pong>(Handle_S_PONG, session, buffer, len); };
		GPacketHandler[PKT_S_SIGNRES] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SIGN_RES>(Handle_S_SIGNRES, session, buffer, len); };
		GPacketHandler[PKT_S_LOGIN] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_LOGIN>(Handle_S_LOGIN, session, buffer, len); };
		GPacketHandler[PKT_S_LOGOUT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_LOGOUT>(Handle_S_LOGOUT, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_EnterRoom>(Handle_S_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_S_ROOM_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Room_List>(Handle_S_ROOMLIST, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN_PLAYER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SpawnPlayer>(Handle_S_SPAWN_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PLAYER_LIST>(Handle_S_PLAYER_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_REMOVE_PLAYER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_RemovePlayer>(Handle_S_REMOVE_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PlayerMove>(Handle_S_PLAYER_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_CUSTOM_SELECT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_CustomSelect>(Handle_S_CUSTOM_SELECT, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN_MONSTER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SpawnMonster>(Handle_S_SPAWN_MONSTER, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN_MONSTER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_DeSpawnMonster>(Handle_S_DESPAWN_MONSTER, session, buffer, len); };
		GPacketHandler[PKT_S_MONSTER_MOVE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_MonsterMove>(Handle_S_MONSTER_MOVE, session, buffer, len); };
		GPacketHandler[PKT_S_SCENE_CHANGE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SceneChange>(Handle_S_SCENE_CHANGE, session, buffer, len); };
		GPacketHandler[PKT_S_MAP_START] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_MapStart>(Handle_S_MAP_START, session, buffer, len); };
		GPacketHandler[PKT_S_MAP_DATA] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_MapData>(Handle_S_MAP_DATA, session, buffer, len); };
		GPacketHandler[PKT_S_MAP_END] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_MapEnd>(Handle_S_MAP_END, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SpawnItem>(Handle_S_SPAWN_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWN_ITEM_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Spawn_Item_List>(Handle_S_SPAWN_ITEM_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_DESPAWN_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_DeSpawnItem>(Handle_S_DESPAWN_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_ADD_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_AddItem>(Handle_S_ADD_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_ADD_ITEM_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_AddItemList>(Handle_S_ADD_ITEM_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_REMOVE_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_RemoveItem>(Handle_S_REMOVE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_READY] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Ready>(Handle_S_READY, session, buffer, len); };
		GPacketHandler[PKT_S_EQUIP_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_EquipItem>(Handle_S_EQUIP_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_USE_ITEM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_UseItem>(Handle_S_USE_ITEM, session, buffer, len); };
		GPacketHandler[PKT_S_MINEABLE_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_MineableList>(Handle_S_MINEABLE_LIST, session, buffer, len); };
		GPacketHandler[PKT_S_DESTROY_MINEABLE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_DestroyMineable>(Handle_S_DESTROY_MINEABLE, session, buffer, len); };
		GPacketHandler[PKT_S_UPDATE_DURABILITY] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_UpdateDurability>(Handle_S_UPDATE_DURABILITY, session, buffer, len); };
		GPacketHandler[PKT_S_PLAY_SOUND] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PlaySound>(Handle_S_PLAY_SOUND, session, buffer, len); };
		GPacketHandler[PKT_S_POSSESSION_RELEASE_FAIL] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PossessionReleaseFail>(Handle_S_POSSESSION_RELEASE_FAIL, session, buffer, len); };
		GPacketHandler[PKT_S_RETURN_ZONE_ACTIVE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_ReturnZoneActive>(Handle_S_RETURN_ZONE_ACTIVE, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYER_RETURNED]   = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PlayerReturned>(Handle_S_PLAYER_RETURNED, session, buffer, len); };
	}

	static bool HandlePacket(std::shared_ptr<Session> session, char* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->GetType()](session, buffer, len);
	}

	template<typename Packet>
	static SendBufferRef MakeSendBuffer(Packet pkt)
	{
		SendBufferRef sendBuffer = std::make_shared<SendBuffer>(pkt.GetSize());
		sendBuffer->CopyData(&pkt, pkt.GetSize());
		sendBuffer->Close(pkt.GetSize());

		return sendBuffer;
	}

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, std::shared_ptr<Session> session, char* buffer, int32 len)
	{
		PacketType* pkt = reinterpret_cast<PacketType*>(buffer);
		return func(session, *pkt);
	}
};