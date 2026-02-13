#pragma once
#include "Timer.h"
#include "NetworkManager.h"

using PacketHandlerFunc = std::function<bool(std::shared_ptr<Session>, char*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Packet ID
enum : uint16
{
	PKT_S_PING = 0,
	PKT_C_PONG = 1,
	PKT_C_PING = 2,
	PKT_S_PONG = 3,
	PKT_C_SIGNUP = 4,
	PKT_S_SIGNRES = 5,
	PKT_C_LOGIN = 6,
	PKT_C_LOGOUT = 7,
	PKT_S_LOGIN = 8,
	PKT_S_LOGOUT = 9,
	PKT_C_CREATE_ROOM = 10,
	PKT_C_UPDATE_ROOM = 11,
	PKT_C_ENTER_ROOM = 12,
	PKT_C_ENTER_SCENE = 13,
	PKT_S_CREATE_ROOM = 14,
	PKT_S_ENTER_ROOM = 15,
	PKT_S_ENTER_SCENE = 16,
	PKT_S_ROOM_LIST = 17,
	PKT_S_SPAWNPLAYER = 18,
	PKT_S_ADDPLAYER = 19,
	PKT_S_PLAYERLIST = 20,
	PKT_S_REMOVEPLAYER = 21,
	PKT_C_PLAYER_INPUT = 22,	// 서버 권위 방식 + 클라 예측 이동
	PKT_S_MOVE = 23,
};

// Custom Handlers
bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len);
bool Handle_S_PING(std::shared_ptr<Session> session, S_Ping& pkt);
bool Handle_S_PONG(std::shared_ptr<Session> session, S_Pong& pkt);
bool Handle_S_SIGNRES(std::shared_ptr<Session> session, S_SIGN_RES& pkt);
bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt);
bool Handle_S_LOGOUT(std::shared_ptr<Session> session, S_LOGOUT& pkt);
bool Handle_S_CREATEROOM(std::shared_ptr<Session> session, S_CreateRoom& pkt);
bool Handle_S_ENTERROOM(std::shared_ptr<Session> session, S_EnterRoom& pkt);
bool Handle_S_ROOMLIST(std::shared_ptr<Session> session, S_Room_List& pkt);
bool Handle_S_MYPLAYER(std::shared_ptr<Session> session, S_SpawnPlayer& pkt);
bool Handle_S_ADDPLAYER(std::shared_ptr<Session> session, S_AddPlayer& pkt);
bool Handle_S_PLAYERLIST(std::shared_ptr<Session> session, S_PLAYER_LIST& pkt);
bool Handle_S_REMOVEPLAYER(std::shared_ptr<Session> session, S_RemovePlayer& pkt);
bool Handle_S_MOVE(std::shared_ptr<Session> session, S_Move& pkt);

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
		GPacketHandler[PKT_S_CREATE_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_CreateRoom>(Handle_S_CREATEROOM, session, buffer, len); };
		GPacketHandler[PKT_S_ENTER_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_EnterRoom>(Handle_S_ENTERROOM, session, buffer, len); };
		GPacketHandler[PKT_S_ROOM_LIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Room_List>(Handle_S_ROOMLIST, session, buffer, len); };
		GPacketHandler[PKT_S_SPAWNPLAYER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_SpawnPlayer>(Handle_S_MYPLAYER, session, buffer, len); };
		GPacketHandler[PKT_S_ADDPLAYER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_AddPlayer>(Handle_S_ADDPLAYER, session, buffer, len); };
		GPacketHandler[PKT_S_PLAYERLIST] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_PLAYER_LIST>(Handle_S_PLAYERLIST, session, buffer, len); };
		GPacketHandler[PKT_S_REMOVEPLAYER] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_RemovePlayer>(Handle_S_REMOVEPLAYER, session, buffer, len); };
		GPacketHandler[PKT_S_MOVE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<S_Move>(Handle_S_MOVE, session, buffer, len); };
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