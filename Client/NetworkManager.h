#pragma once
#include <memory>

class TcpClientService;
class ServerSession;

class NetworkManager
{
public:
	NetworkManager();
	~NetworkManager();

	void ServiceStart();
	void Update();

private:
	std::shared_ptr<TcpClientService>	client_service;
	std::shared_ptr<ServerSession>		server_session;
};

