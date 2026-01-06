#pragma once
#include "NetAddress.h"
#include "IOCP.h"
#include "Listener.h"

enum SERVICE_TYPE
{
	Server,
	Client,
};

using CreateSessionFunc = std::function<std::shared_ptr<Session>(void)>;

class Service : public std::enable_shared_from_this<Service>
{
public:
	Service(SERVICE_TYPE, NetAddress, CreateSessionFunc, int32);
	~Service();

public:
	IOCP&										GetIocpCore() { return iocp_core; }
	NetAddress									GetNetAddress() { return net_address; }
	std::map<SOCKET, std::shared_ptr<Session>>&	GetSesssions() { return sessions; }

	std::shared_ptr<Session>					CreateSession();
	void										AddSession(std::shared_ptr<Session> session);
	void										ReleaseSession(std::shared_ptr<Session> session);

	std::shared_ptr<Service>					GetServiceRef() { return shared_from_this(); }
	SERVICE_TYPE								GetServiceType() { return service_type; }
	int32										GetCurrentSessionCount() { return current_session_count; }
	int32										GetMaxSessionCount() { return max_session_count; }

protected:
	IOCP										iocp_core;
	NetAddress									net_address;
	SERVICE_TYPE								service_type;
	CreateSessionFunc							create_session_func;

	int32										current_session_count;
	int32										max_session_count;
	std::map<SOCKET, std::shared_ptr<Session>>	sessions;
	std::mutex									lock;
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