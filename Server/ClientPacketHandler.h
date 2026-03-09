#pragma once

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
	PKT_S_Spawn_MONSTER,
	PKT_S_MONSTER_MOVE,
};

// Custom Handlers
bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len);
bool Handle_C_PING(shared_ptr<Session> session, C_Ping& pkt);
bool Handle_C_PONG(shared_ptr<Session> session, C_Pong& pkt);
bool Handle_C_LOGIN(shared_ptr<Session> session, C_LOGIN& pkt);
bool Handle_C_LOGOUT(shared_ptr<Session> session, C_LOGOUT& pkt);
bool Handle_C_SIGNUP(shared_ptr<Session> session, C_SIGNUP& pkt);
bool Handle_C_CREATE_ROOM(shared_ptr<Session> session, C_CreateRoom& pkt);
bool Handle_C_UPDATE_ROOM(shared_ptr<Session> session, C_UpdateRoom& pkt);
bool Handle_C_ENTER_ROOM(shared_ptr<Session> session, C_EnterRoom& pkt);
bool Handle_C_LEAVE_ROOM(shared_ptr<Session> session, C_LeaveRoom& pkt);
bool Handle_C_PLAYER_INPUT(std::shared_ptr<Session> session, C_Input& pkt);
bool Handle_C_CUSTOM_SELECT(std::shared_ptr<Session> session, C_CustomSelect& pkt);
bool Handle_C_SCENE_CHANGE(std::shared_ptr<Session> session, C_SceneChange& pkt);

class CClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		GPacketHandler[PKT_C_PING] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_Ping>(Handle_C_PING, session, buffer, len); };
		GPacketHandler[PKT_C_PONG] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_Pong>(Handle_C_PONG, session, buffer, len); };
		GPacketHandler[PKT_C_LOGIN] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };	
		GPacketHandler[PKT_C_LOGOUT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_LOGOUT>(Handle_C_LOGOUT, session, buffer, len); };
		GPacketHandler[PKT_C_SIGNUP] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_SIGNUP>(Handle_C_SIGNUP, session, buffer, len); };	
		GPacketHandler[PKT_C_CREATE_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_CreateRoom>(Handle_C_CREATE_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_UPDATE_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_UpdateRoom>(Handle_C_UPDATE_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_ENTER_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_EnterRoom>(Handle_C_ENTER_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_LEAVE_ROOM] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_LeaveRoom>(Handle_C_LEAVE_ROOM, session, buffer, len); };
		GPacketHandler[PKT_C_PLAYER_INPUT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_Input>(Handle_C_PLAYER_INPUT, session, buffer, len); };
		GPacketHandler[PKT_C_CUSTOM_SELECT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_CustomSelect>(Handle_C_CUSTOM_SELECT, session, buffer, len); };
		GPacketHandler[PKT_C_SCENE_CHANGE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_SceneChange>(Handle_C_SCENE_CHANGE, session, buffer, len); };
	}

	static bool HandlePacket(shared_ptr<Session> session, char* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->GetType()](session, buffer, len);
	}

	template<typename Packet>
	static SendBufferRef MakeSendBuffer(Packet pkt)
	{
		SendBufferRef sendBuffer = make_shared<SendBuffer>(pkt.GetSize());
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

