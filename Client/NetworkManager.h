#pragma once
#include  "JitterMeasurer.h"

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

	void ServiceStart(std::wstring address, uint16 port);
	void Tick(float time);

	std::shared_ptr<TcpClientService> GetClientService() const { return client_service; }
	CJitterMeasurer* GetJitterMeasurer() const { return jitter_measurer.get(); }

private:
	std::shared_ptr<TcpClientService>	client_service;
	std::unique_ptr<CJitterMeasurer>    jitter_measurer;
};

