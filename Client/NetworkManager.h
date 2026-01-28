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
	void Tick(float time);

	std::shared_ptr<TcpClientService> ClientService() const { return client_service; }

private:
	std::shared_ptr<TcpClientService>	client_service;
};

