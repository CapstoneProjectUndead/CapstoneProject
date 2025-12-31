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

	CloseSocket();
}

bool Listener::BeginAccept(shared_ptr<TcpServerService> service)
{
	server_service = service;
	if (false == server_service->GetIocpCore().Register(reinterpret_cast<HANDLE>(GetHandle()), 9999))
	{
		cout << "IOCP register fail" << endl;
		LogUtil::error_display(GetLastError());
		return false;
	}

	if (false == BindListen(server_service->GetNetAddress()))
		return false;

	int32 sessionCount = server_service->GetMaxSessionCount();
	for (int32 i = 0; i < sessionCount; ++i)
	{
		OVER_EXP* acceptOver = new OVER_EXP;
		acceptOver->comp_type = COMP_TYPE::OP_ACCEPT;
		acceptOver->listener_ref = this;
		DoAccept(acceptOver);
	}

	return true;
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

void Listener::DoAccept(OVER_EXP* acceptOver)
{
	shared_ptr<Session> session = server_service->CreateSession();

	acceptOver->session_ref = session;
	accept_overs.push_back(acceptOver);

	if (false == SocketHelper::AcceptEx(listen_socket, session->socket, acceptOver->recv_buf.data(), 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, NULL, &acceptOver->wsa_over))
	{
		int err = WSAGetLastError();
		if (err != WSA_IO_PENDING)
		{
			LogUtil::error_display(err);
			DoAccept(acceptOver);
		}
	}
}

void Listener::HandleAccept(OVER_EXP* acceptOver)
{
	shared_ptr<Session> session = acceptOver->session_ref;
	assert(session);

	if (false == SocketHelper::SetUpdateAcceptSocket(session->GetSocket(), listen_socket))
	{
		cout << "Socket update error" << endl;
		LogUtil::error_display(::WSAGetLastError());
		DoAccept(acceptOver);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddr = sizeof(sockAddress);
	if (SOCKET_ERROR == ::getpeername(session->GetSocket(), OUT reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddr))
	{
		DoAccept(acceptOver);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));
	session->HandleConnect();
	DoAccept(acceptOver);
}

void Listener::CloseSocket()
{
	SocketHelper::Close(listen_socket);
}
