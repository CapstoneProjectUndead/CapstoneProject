#pragma once
#include "Core.h"

#include <ServerEngine/global.h>
#include <ServerEngine/externs.h>
#include <ServerEngine/ThreadManager.h>
#include <ServerEngine/Service.h>

#define PORT_NUM 7777

class NetworkService
{
public:
	NetworkService();
	~NetworkService();

	void ServiceStart();

private:
	unique_ptr<TcpClientService> client_service;
};

