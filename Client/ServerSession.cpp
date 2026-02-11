#include "stdafx.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"
#include "ServerSessionManager.h"

CServerSession::CServerSession()
{
}

CServerSession::~CServerSession()
{
}

void CServerSession::OnConnected()
{
	CServerSessionManager::GetInstance().SetServerSession(std::static_pointer_cast<CServerSession>(GetSessionRef()));

#ifdef SCENE_TEST
	C_LOGIN loginPkt;
	SendBufferRef sendBuffer = CServerPacketHandler::MakeSendBuffer<C_LOGIN>(loginPkt);
	DoSend(sendBuffer);
#endif
}

void CServerSession::OnDisconnected()
{
	CServerSessionManager::GetInstance().SetServerSession(nullptr);
}

void CServerSession::ProcessPacket(std::shared_ptr<Session> session, char* buf, int32 pktSize)
{
	CServerPacketHandler::HandlePacket(session, buf, pktSize);
}
