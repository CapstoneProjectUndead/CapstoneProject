#pragma once
#include "OVER_EXP.h"

class TcpServerService;

class Listener
{
public:
	Listener();
	~Listener();

	void SetServerService(TcpServerService* service) { server_service = service; }
	TcpServerService* GetServerService() { return server_service; }

	bool BindListen(class NetAddress address);
	bool DoAccept();
	void HandleAccept(OVER_EXP*);

	SOCKET GetHandle() { return listen_socket; }

private:
	SOCKET				listen_socket;
	TcpServerService*	server_service;
	vector<OVER_EXP*>	accept_overs;
};

