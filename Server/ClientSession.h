#pragma once
#include <ServerEngine\Session.h>

class ClientSession :
    public Session
{
public:
	ClientSession();
	~ClientSession();

	virtual void			OnConnected() override;
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() override;

	virtual void ProcessPacket(Session*, char*, int32 pktSize) override;
};

