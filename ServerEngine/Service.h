#pragma once
#include "NetAddress.h"
#include "IOCP.h"
#include "Listener.h"

enum SERVICE_TYPE
{
	Server,
	Client,
};

using CreateSessionFunc = std::function<Session*(void)>;

class Service
{
public:
	Service(SERVICE_TYPE, NetAddress, CreateSessionFunc, int32);
	~Service();

public:
	IOCP&					GetIocpCore() { return iocp_core; }
	NetAddress				GetNetAddress() { return net_address; }
	map<SOCKET, Session*>&	GetSesssions() { return sessions; }

	Session*				CreateSession();
	void					AddSession(Session* session);
	void					ReleaseSession(Session* session);

	SERVICE_TYPE			GetServiceType() { return service_type; }
	int32					GetCurrentSessionCount() { return current_session_count; }
	int32					GetMaxSessionCount() { return max_session_count; }

protected:
	IOCP					iocp_core;
	NetAddress				net_address;
	SERVICE_TYPE			service_type;
	CreateSessionFunc		create_session_func;

	int32					current_session_count;
	int32					max_session_count;
	map<SOCKET, Session*>	sessions;
	mutex					lock;
};

// ServerService

class TcpServerService : public Service
{
public:
	TcpServerService(NetAddress, CreateSessionFunc, int32);
	~TcpServerService();

	bool StartServer();

private:
	Listener listener;
};

// ClientService

class TcpClientService : public Service
{
public:
	TcpClientService(NetAddress, CreateSessionFunc, int32);
	~TcpClientService();

	bool StartClientService();

private:

};