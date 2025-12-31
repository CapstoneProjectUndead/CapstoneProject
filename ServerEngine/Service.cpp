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
}

shared_ptr<Session>	Service::CreateSession()
{
	shared_ptr<Session> session = create_session_func();
	session->SetServiceRef(shared_from_this());

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

void Service::AddSession(shared_ptr<Session> session)
{
	lock_guard<mutex> lg(lock);
	current_session_count++;
	session->SetState(S_STATE::ST_ALLOC);
	sessions[session->GetSocket()] = session;
}

void Service::ReleaseSession(shared_ptr<Session> session)
{
	lock_guard<mutex> lg(lock);
	current_session_count--;
	ASSERT_CRASH(0 != sessions.erase(session->GetSocket()));
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
	shared_ptr<TcpServerService> serverService = static_pointer_cast<TcpServerService>(shared_from_this());

	if (false == listener.BeginAccept(serverService))
		return false;

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

	shared_ptr<Session> session = CreateSession();
	if (false == session->DoConnect())
		return false;

	return true;
}