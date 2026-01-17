#include "stdafx.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"

CServerSession::CServerSession()
{
}

CServerSession::~CServerSession()
{
}

void CServerSession::OnConnected()
{
	C_LOGIN loginPkt;

	SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);
	DoSend(sendBuffer);
}

void CServerSession::OnDisconnected()
{
}

void CServerSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CServerPacketHandler::HandlePacket(session, buf, pktSize);
}
