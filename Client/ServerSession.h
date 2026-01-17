#pragma once
#include <ServerEngine/Session.h>

class CServerSession :
    public Session
{
public:
	CServerSession();
	~CServerSession();

	virtual void			OnConnected() override;
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() override;

	virtual void			ProcessPacket(std::shared_ptr<Session>, char*, int32 pktSize) override;
};

