#pragma once
#include "Core.h"

#include <ServerEngine/global.h>
#include <ServerEngine/ThreadManager.h>
#include <ServerEngine/Service.h>

#include <protocol.h>

class ServerSession;

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	void ServiceStart();

private:
	shared_ptr<TcpClientService> client_service;
	shared_ptr<ServerSession>	 server_session;
};

