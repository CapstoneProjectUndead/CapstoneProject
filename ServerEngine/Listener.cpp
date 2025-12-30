#include "pch.h"
#include "Listener.h"
#include "SocketHelper.h"
#include "NetAddress.h"
#include "Session.h"
#include "Service.h"

Listener::Listener()
	: server_service(nullptr)
{
	listen_socket = SocketHelper::CreateSocket();
	if (INVALID_SOCKET == listen_socket)
	{
		LogUtil::error_display(GetLastError());
		assert(nullptr);
	}
}

Listener::~Listener()
{
	for (OVER_EXP* acceptEvent : accept_overs)
	{
		delete acceptEvent;
	}
}

bool Listener::BindListen(NetAddress address)
{
	if (SocketHelper::SetReuseAddress(listen_socket, true) == false)
		return false;

	// 소켓 종료 시, 잔류 데이터 처리 방식(Linger 옵션)을 설정하는 함수
	if (SocketHelper::SetLinger(listen_socket, 0, 0) == false)
		return false;

	if (SocketHelper::Bind(listen_socket, address) == false)
	{
		cout << "[bind Error] ";
		LogUtil::error_display(WSAGetLastError());
		return false;
	}

	if (SocketHelper::Listen(listen_socket) == false)
	{
		cout << "[listen Error] ";
		LogUtil::error_display(WSAGetLastError());
		return false;
	}

	return true;
}

bool Listener::DoAccept()
{
	Session* session = GetServerService()->CreateSession();

	OVER_EXP* accept_over = new OVER_EXP;
	accept_over->comp_type = COMP_TYPE::OP_ACCEPT;
	accept_over->listener_ref = this;
	accept_over->session_ref = session;

	accept_overs.push_back(accept_over);

	if (false == SocketHelper::AcceptEx(listen_socket, session->socket, accept_over->recv_buf.data(), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &accept_over->wsa_over))
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			LogUtil::error_display(err);
			delete session;
			return false;
		}
	}

	return true;
}

void Listener::HandleAccept(OVER_EXP* overEXP)
{
	Session* session = overEXP->session_ref;
	assert(session);

	if (false == SocketHelper::SetUpdateAcceptSocket(session->GetSocket(), listen_socket))
	{
		cout << "Socket update error" << endl;
		LogUtil::error_display(::WSAGetLastError());
		DoAccept();
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		DoAccept();
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));
	session->HandleConnect();
	DoAccept();
}
