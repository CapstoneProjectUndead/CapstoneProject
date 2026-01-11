#pragma once
#include <memory>

class TcpClientService;
class CServerSession;

class CNetworkManager
{
private:
	CNetworkManager();
	CNetworkManager(const CNetworkManager&) = delete;

public:
	~CNetworkManager();

	static CNetworkManager& GetInstance() {
		static CNetworkManager instance;
		return instance;
	}

	void ServiceStart();
	void Update();

private:
	std::shared_ptr<TcpClientService>	client_service;
	std::shared_ptr<CServerSession>		server_session;
};

