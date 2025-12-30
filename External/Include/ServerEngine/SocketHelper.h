#pragma once

// 소켓 관련 함수를 제공하는 클래스

class SocketHelper
{
public:
	static LPFN_CONNECTEX		ConnectEx;
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

public:
	static void		Init();
	static void		Clear();

	static bool		BindWindowsFunction(SOCKET socket, GUID guid, LPVOID* fn);
	static SOCKET	CreateSocket();

	// 소켓 종료 시, 잔류 데이터 처리 방식(Linger 옵션)을 설정하는 함수
	static bool		SetLinger(SOCKET socket, uint16 onoff, uint16 linger);

	// 서버가 강제 종료되었을 때도 같은 포트 번호를 바로 다시 사용할 수 있도록 해주는 옵션
	static bool		SetReuseAddress(SOCKET socket, bool flag);

	static bool		SetRecvBufferSize(SOCKET socket, int32 size);
	static bool		SetSendBufferSize(SOCKET socket, int32 size);

	// Nagle 알고리즘 On/Off
	static bool		SetTcpNoDelay(SOCKET socket, bool flag);

	// AcceptEx()로 연결된 소켓을 일반적인 소켓처럼 사용할 수 있게 만들어주는 필수 작업
	static bool		SetUpdateAcceptSocket(SOCKET socket, SOCKET listenSocket);

	static bool		Bind(SOCKET socket, class NetAddress netAddr);
	static bool		BindAnyAddress(SOCKET socket, uint16 port);
	static bool		Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static void		Close(SOCKET& socket);
};

template<typename T>
static inline bool SetSockOpt(SOCKET socket, int32 level, int32 optName, T optVal)
{
	return SOCKET_ERROR != ::setsockopt(socket, level, optName, reinterpret_cast<char*>(&optVal), sizeof(T));
}

// 1. 서버 주소 정보를 담을 구조체 선언

// 2. 구조체의 모든 바이트를 0으로 초기화 (예상치 못한 쓰레기값 방지)

// 3. 주소 체계(AF_INET = IPv4)를 설정

// 4. 포트 번호 설정 (네트워크 바이트 순서로 변환)

// 5. IP 주소 설정 (INADDR_ANY는 모든 NIC(네트워크 인터페이스)에서 수신 허용)

// 6. 바인딩: 서버 소켓에 IP와 포트 설정

// 7. 리슨 상태로 전환: 클라이언트의 접속을 수신 대기