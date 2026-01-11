#include "stdafx.h"
#include "ServerPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

bool Handle_INVALID(std::shared_ptr<Session> session, char* buffer, int32 len)
{
	//std::cout << "정의 되지 않은 패킷 ID 입니다!" << std::endl;
	assert(nullptr);
	return false;
}

bool Handle_S_LOGIN(std::shared_ptr<Session> session, S_LOGIN& pkt)
{

	return true;
}
