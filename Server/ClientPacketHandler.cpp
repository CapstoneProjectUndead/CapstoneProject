#include "pch.h"
#include "ClientPacketHandler.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX]{};

bool Handle_INVALID(shared_ptr<Session> session, char* buffer, int32 len)
{
	cout << "정의 되지 않은 패킷 ID 입니다!" << endl;
	assert(nullptr);
	return false;
}

bool Handle_C_LOGIN(std::shared_ptr<Session> session, C_LOGIN& pkt)
{
	return true;
}
