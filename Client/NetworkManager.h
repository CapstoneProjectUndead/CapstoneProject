#pragma once
#include <memory>

class TcpClientService;
class CServerSession;

class CNetworkManager
{
public:
	CNetworkManager();
	~CNetworkManager();

	void ServiceStart();
	void Update();

private:
	std::shared_ptr<TcpClientService>	client_service;
	std::shared_ptr<CServerSession>		server_session;
};

