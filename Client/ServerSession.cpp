#include "stdafx.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "SessionManager.h"

CServerSession::CServerSession()
{
}

CServerSession::~CServerSession()
{
}

void CServerSession::OnConnected()
{
	CSessionManager::GetInstance().SetServerSession(std::static_pointer_cast<CServerSession>(GetSessionRef()));

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
