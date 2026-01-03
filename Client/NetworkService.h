#pragma once
#include "Core.h"

#include <ServerEngine/global.h>
#include <ServerEngine/ThreadManager.h>
#include <ServerEngine/Service.h>

#include <protocol.h>

class ServerSession;

class NetworkService
{
public:
	NetworkService();
	~NetworkService();

	void ServiceStart();

private:
	shared_ptr<TcpClientService> client_service;
	shared_ptr<ServerSession>	 server_session;
};

