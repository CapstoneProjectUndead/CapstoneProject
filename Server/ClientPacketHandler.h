#pragma once

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
	PKT_S_LOGIN = 7,
	PKT_S_LOGINFAIL = 8,
	PKT_S_SPAWNPLAYER = 9,
	PKT_S_ADDPLAYER = 10,
	PKT_S_PLAYERLIST = 11,
	PKT_S_REMOVEPLAYER = 12,
	PKT_C_PLAYERINPUT = 13,
	PKT_S_MOVE = 14,
};

// Custom Handlers
bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len);
bool Handle_C_PING(shared_ptr<Session> session, C_Ping& pkt);
bool Handle_C_PONG(shared_ptr<Session> session, C_Pong& pkt);
bool Handle_C_LOGIN(shared_ptr<Session> session, C_LOGIN& pkt);

bool Handle_C_PLAYERINPUT(std::shared_ptr<Session> session, C_Input& pkt);

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
		GPacketHandler[PKT_C_PLAYERINPUT] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_Input>(Handle_C_PLAYERINPUT, session, buffer, len); };
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

