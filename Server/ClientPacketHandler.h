#pragma once

using PacketHandlerFunc = std::function<bool(std::shared_ptr<Session>, char*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

// Packet ID
enum : uint16
{
	PKT_C_SIGNUP = 0,
	PKT_S_SIGNRES = 1,
	PKT_C_LOGIN = 2,
	PKT_S_LOGIN = 3,
	PKT_S_LOGINFAIL = 4,
	PKT_S_SPAWNPLAYER = 5,
	PKT_S_ADDPLAYER = 6,
	PKT_S_PLAYERLIST = 7,
	PKT_S_REMOVEPLAYER = 8,
	PKT_C_MOVE = 9,
	PKT_S_MOVE = 10,
};

// Custom Handlers
bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len);
bool Handle_C_LOGIN(std::shared_ptr<Session> session, C_LOGIN& pkt);
bool Handle_C_MOVE(std::shared_ptr<Session> session, C_Move& pkt);

class CClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;

		GPacketHandler[PKT_C_LOGIN] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_LOGIN>(Handle_C_LOGIN, session, buffer, len); };	
		GPacketHandler[PKT_C_MOVE] = [](std::shared_ptr<Session> session, char* buffer, int32 len) { return HandlePacket<C_Move>(Handle_C_MOVE, session, buffer, len); };
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

