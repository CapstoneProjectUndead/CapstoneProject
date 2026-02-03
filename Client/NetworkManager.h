#pragma once
#include "Player.h"
#include "JitterMeasurer.h"

class TcpClientService;
class CServerSession;
class CLagSimulator;

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

public:
	// [¼ö½ÅºÎ]
	void OnRecvOpponentPos(const OpponentFrameHistory& packet);
	void UpdateDelayPacket();

private:
	std::shared_ptr<TcpClientService>	client_service;
	std::unique_ptr<CJitterMeasurer>    jitter_measurer;
	std::unique_ptr<CLagSimulator>		lag_simulator;
};

