#include "pch.h"
#include "SocketHelper.h"
#include "NetAddress.h"

LPFN_CONNECTEX		SocketHelper::ConnectEx = nullptr;
LPFN_DISCONNECTEX	SocketHelper::DisconnectEx = nullptr;
LPFN_ACCEPTEX		SocketHelper::AcceptEx = nullptr;

void SocketHelper::Init()
{
	// 초기화
	WSADATA wsaData;
	ASSERT_CRASH(::WSAStartup(MAKEWORD(2, 2), OUT & wsaData) == 0);

	// 런타임에 주소 얻어오는 API
	// 이 작업은 서버 초기화 시 1회만 수행하고, 
	// 이후에는 캐시된 함수 포인터를 직접 사용하여 호출 성능 향상.
	SOCKET socket = CreateSocket();
	ASSERT_CRASH(BindWindowsFunction(socket, WSAID_CONNECTEX, reinterpret_cast<LPVOID*>(&ConnectEx)));
	ASSERT_CRASH(BindWindowsFunction(socket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)));
	ASSERT_CRASH(BindWindowsFunction(socket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)));

	Close(socket);
}

void SocketHelper::Clear()
{
	::WSACleanup();
}

bool SocketHelper::BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
	DWORD bytes = 0;
	return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn), OUT & bytes, NULL, NULL);
}

SOCKET SocketHelper::CreateSocket()
{
	return ::WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
}

bool SocketHelper::SetLinger(SOCKET socket, uint16 onoff, uint16 linger)
{
	// 소켓을 close()하거나 closesocket()할 때
	// 아직 전송되지 않은 데이터가 있을 경우 그것을 어떻게 처리할지 결정

	LINGER option;
	option.l_onoff = onoff;
	option.l_linger = linger;
	return SetSockOpt(socket, SOL_SOCKET, SO_LINGER, option);
}

bool SocketHelper::SetReuseAddress(SOCKET socket, bool flag)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_REUSEADDR, flag);
}

bool SocketHelper::SetRecvBufferSize(SOCKET socket, int32 size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_RCVBUF, size);
}

bool SocketHelper::SetSendBufferSize(SOCKET socket, int32 size)
{
	return SetSockOpt(socket, SOL_SOCKET, SO_SNDBUF, size);
}

bool SocketHelper::SetTcpNoDelay(SOCKET socket, bool flag)
{
	return SetSockOpt(socket, SOL_SOCKET, TCP_NODELAY, flag);
}

bool SocketHelper::SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket)
{
	// 새 소켓이 어떤 listen 소켓을 통해 accept되었는지 커널에 알려줌
	// 이걸 해야
	// shutdown(), getsockname(), getpeername() 등이 정상 동작

	return SetSockOpt(socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, listenSocket);
}

bool SocketHelper::Bind(SOCKET socket, NetAddress netAddr)
{
	return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&netAddr.GetSockAddress()), sizeof(SOCKADDR_IN));
}

bool SocketHelper::BindAnyAddress(SOCKET socket, uint16 port)
{
	SOCKADDR_IN myAddress;
	myAddress.sin_family = AF_INET;
	myAddress.sin_addr.s_addr = ::htonl(INADDR_ANY);
	myAddress.sin_port = ::htons(port);

	return SOCKET_ERROR != ::bind(socket, reinterpret_cast<const SOCKADDR*>(&myAddress), sizeof(myAddress));
}

bool SocketHelper::Listen(SOCKET socket, int32 backlog)
{
	return SOCKET_ERROR != ::listen(socket, backlog);
}

void SocketHelper::Close(SOCKET& socket)
{
	if (socket != INVALID_SOCKET)
		::closesocket(socket);
	socket = INVALID_SOCKET;
}
