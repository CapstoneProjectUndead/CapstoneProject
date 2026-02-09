#pragma once
#include <ServerEngine/Session.h>

class CUser;

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

public:
	std::shared_ptr<CUser> GetUser() const { return user; }
	void SetUser(std::shared_ptr<CUser> _user) { user = _user; }

private:
	std::shared_ptr<CUser> user;
};

