#include "pch.h"
#include "ClientSession.h"

ClientSession::ClientSession()
{
}

ClientSession::~ClientSession()
{
}

void ClientSession::OnConnected()
{
	cout << "ClientSession 접속 성공!" << endl;
}

void ClientSession::OnDisconnected()
{
}

void ClientSession::ProcessPacket(Session*, char*, int32 pktSize)
{
}
