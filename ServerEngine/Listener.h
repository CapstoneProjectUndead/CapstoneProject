#pragma once
#include "OVER_EXP.h"

class TcpServerService;

class Listener
{
public:
	Listener();
	~Listener();
	
	bool BeginAccept(std::shared_ptr<TcpServerService> service);
	bool BindListen(class NetAddress address);
	void DoAccept(OVER_EXP* acceptOver);
	void HandleAccept(OVER_EXP* acceptOver);

	SOCKET GetHandle() { return listen_socket; }
	void SetServerService(std::shared_ptr<TcpServerService> service) { server_service = service; }

	void CloseSocket();

private:
	SOCKET								listen_socket;
	std::shared_ptr<TcpServerService>	server_service;
	std::vector<OVER_EXP*>				accept_overs;
};

