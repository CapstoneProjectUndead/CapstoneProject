#include "pch.h"
#include "Session.h"
#include "SocketHelper.h"
#include "Service.h"
#include "SendBuffer.h"

Session::Session()
	: prev_remain(0)
	, state(S_STATE::ST_FREE)
{
	socket = SocketHelper::CreateSocket();
	connect_over.comp_type = COMP_TYPE::OP_CONNECT;
	disconnect_over.comp_type = COMP_TYPE::OP_DISCONNECT;
}

Session::~Session()
{
	SocketHelper::Close(socket);
}

bool Session::DoConnect()
{
	if (IsConnected())
		return false;

	if (SocketHelper::SetReuseAddress(socket, true) == false)
		return false;

	if (SocketHelper::BindAnyAddress(socket, 0/*포트 번호 남는거*/) == false)
		return false;

	DWORD numOfBytes = 0;
	SOCKADDR_IN targetAddr = GetServiceRef()->GetNetAddress().GetSockAddress();
	connect_over.session_ref = GetSessionRef();

	if (false == SocketHelper::ConnectEx(socket, reinterpret_cast<SOCKADDR*>(&targetAddr), sizeof(targetAddr), nullptr, 0, &numOfBytes, &connect_over.wsa_over))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LogUtil::error_display(errorCode);
			return false;
		}
	}

	return true;
}

void Session::DoDisconnect(const WCHAR* cause)
{
	if (is_connect.exchange(false) == false)
		return;

	// temp
	std::wcout << L"Disconnect : " << cause << endl;

	disconnect_over.session_ref = GetSessionRef();
	if (false == SocketHelper::DisconnectEx(socket, &disconnect_over.wsa_over, TF_REUSE_SOCKET, 0))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			LogUtil::error_display(errorCode);
			return;
		}
	}
}

void Session::DoRecv()
{
	if (IsConnected() == false)
		return;

	DWORD recv_flag = 0;
	memset(&recv_over.wsa_over, 0, sizeof(recv_over.wsa_over));
	recv_over.wsabuf.len = recv_over.BufferSize() - prev_remain;
	recv_over.wsabuf.buf = recv_over.recv_buf.data() + prev_remain;
	recv_over.session_ref = GetSessionRef();

	WSARecv(socket, &recv_over.wsabuf, 1, 0, &recv_flag,
		&recv_over.wsa_over, 0);
}

void Session::DoSend(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	{
		lock_guard<mutex> lg(lock);

		send_queue.push(sendBuffer);

		// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어준다   
		if (send_registered.exchange(true) == false)
			registerSend = true;
		else
			return;
	}

	if (registerSend)
		RegisterSend();
}

void Session::RegisterSend()
{
	lock_guard<mutex> lg(lock);

	vector<WSABUF> wsaBufs;
	wsaBufs.reserve(send_queue.size());

	OVER_EXP* send_over = new OVER_EXP(send_queue, wsaBufs);

	// OVER_EXP를 동적할당하는 구조에서는
	// 아래와 같이 Session의 수명관리가 필수는
	// 아니지만 일단 이렇게 작성했다.
	send_over->session_ref = GetSessionRef();

	DWORD numOfBytes = 0;
	if (SOCKET_ERROR == ::WSASend(socket, wsaBufs.data(), static_cast<DWORD>(wsaBufs.size()), OUT & numOfBytes, 0, &send_over->wsa_over, nullptr))
	{
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING)
		{
			cout << "Send 문제 발생" << endl;
			LogUtil::error_display(errorCode);
			send_registered.store(false);
			delete send_over;
		}
	}
}

void Session::HandleConnect()
{
	// RELEASE_REF
	connect_over.session_ref = nullptr;

	// 연결 성공
	is_connect.store(true);

	// 콘텐츠쪽에서 재정의
	OnConnected();

	// TODO : Service class 에서 해당 session을 관리하는 컨테이너에 넣는다.
	GetServiceRef()->AddSession(GetSessionRef());

	// 수신하기
	DoRecv();
}

void Session::HandleDisConnect()
{
	// RELEASE_REF
	disconnect_over.session_ref = nullptr;

	OnDisconnected(); // 컨텐츠 코드에서 재정의

	GetServiceRef()->ReleaseSession(GetSessionRef());
}

void Session::HandleRecv(int numOfBytes)
{
	// RELEASE_REF
	recv_over.session_ref = nullptr;

	if (0 == numOfBytes)
	{
		DoDisconnect(L"Recv 0 Byte");
		return;
	}

	if (false == OnRecv((BYTE*)recv_over.recv_buf.data(), numOfBytes))
		return;

	DoRecv();
}

bool Session::OnRecv(BYTE* buffer, int32 numOfBytes)
{
	int remainData = numOfBytes + prev_remain;
	char* p = recv_over.recv_buf.data();

	while (remainData >= sizeof(PacketHeader))
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(p);
		int packet_size = header->GetSize();

		if (packet_size <= 0 || packet_size > recv_over.BufferSize())
		{
			// 비정상 패킷 감지 (해킹 or 버그)
			DoDisconnect(L"recv error");
			return false;
		}

		// 패킷 사이즈 보다 numOfBytes가 작다면 break하고
		// 다음 recv 때 처리.
		if (remainData < packet_size)
			break;

		// 컨텐츠단에서 처리
		ProcessPacket(shared_from_this(), p, packet_size);
		p += packet_size;
		remainData -= packet_size;
	}

	// 남은 데이터 복사
	prev_remain = remainData;
	if (remainData > 0)
		memmove(recv_over.recv_buf.data(), p, remainData);

	return true;
}

void Session::HandleSend(int numOfBytes)
{
	if (numOfBytes == 0)
	{
		DoDisconnect(L"Send 0 Byte");
		return;
	}

	// 콘텐츠 
	OnSend(numOfBytes);

	lock.lock();
	if (send_queue.empty())
	{
		send_registered.store(false);
		lock.unlock();
	}
	else
	{
		lock.unlock();
		RegisterSend();
	}
}
