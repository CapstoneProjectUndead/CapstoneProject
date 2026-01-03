#pragma once
#include "Core.h"
#include <ServerEngine/global.h>
#include <ServerEngine/Session.h>

class ServerSession :
    public Session
{
public:
	ServerSession();
	~ServerSession();

	virtual void			OnConnected() override;
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() override;

	virtual void ProcessPacket(Session*, char*, int32 pktSize) override;
};

