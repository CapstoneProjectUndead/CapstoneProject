#include "pch.h"
#include "Service.h"
#include "Session.h"

Service::Service(SERVICE_TYPE type, NetAddress address, CreateSessionFunc func, int32 maxCount)
	: service_type(type)
	, net_address(address)
	, create_session_func(func)
	, current_session_count(0)
	, max_session_count(maxCount)
{
}

Service::~Service()
{
	for (auto& value : sessions)
	{
		delete value.second;
		value.second = nullptr;
	}
}

Session* Service::CreateSession()
{
	Session* session = create_session_func();
	session->SetService(this);

	// 세션의 소켓을 IOCP에 등록한다.
	if (false == iocp_core.Register(reinterpret_cast<HANDLE>(session->GetSocket()),
		static_cast<ULONG_PTR>(session->GetSocket())))
	{
		cout << "socket register fail" << endl;
		LogUtil::error_display(::WSAGetLastError());
		return nullptr;
	}

	return session;
}

void Service::AddSession(Session* session)
{
	lock_guard<mutex> lg(lock);
	current_session_count++;
	session->SetState(S_STATE::ST_ALLOC);
	sessions[session->GetSocket()] = session;
}

void Service::ReleaseSession(Session* session)
{
	lock_guard<mutex> lg(lock);
	current_session_count--;
	ASSERT_CRASH(0 != sessions.erase(session->GetSocket()));
	delete session;
	session = nullptr;
}


//==================
// TcpServerService
//==================

TcpServerService::TcpServerService(NetAddress address, CreateSessionFunc func, int32 maxCount)
	: Service(SERVICE_TYPE::Server, address, func, maxCount)
{

}

TcpServerService::~TcpServerService()
{

}

bool TcpServerService::StartServer()
{
	listener.SetServerService(this);

	if (false == iocp_core.Register(reinterpret_cast<HANDLE>(listener.GetHandle()), 9999))
	{
		cout << "IOCP register fail" << endl;
		LogUtil::error_display(GetLastError());
		return false;
	}

	if (false == listener.BindListen(net_address))
		return false;

	for (int32 i = 0; i < max_session_count; ++i)
	{
		if (false == listener.DoAccept())
			return false;
	}

	return true;
}

//==================
// TcpClientService
//==================

TcpClientService::TcpClientService(NetAddress address, CreateSessionFunc func, int32 maxCount)
	: Service(SERVICE_TYPE::Client, address, func, maxCount)
{

}

TcpClientService::~TcpClientService()
{
}

bool TcpClientService::StartClientService()
{
	const int32 sessionCount = GetMaxSessionCount();

	Session* session = CreateSession();
	if (false == session->DoConnect())
		return false;

	return true;
}