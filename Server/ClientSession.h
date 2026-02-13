#pragma once
#include <ServerEngine\Session.h>

class CPlayer;
class CUser;

class CClientSession :
    public Session
{
public:
	CClientSession();
	~CClientSession();

	virtual void			OnConnected() override;
	virtual void			OnSend(int32 len) {}
	virtual void			OnDisconnected() override;

	virtual void			ProcessPacket(std::shared_ptr<Session>, char*, int32 pktSize) override;

public:
	shared_ptr<CUser> GetUser() { return user; }
	void SetUser(shared_ptr<CUser> _user) { user = _user; }

private:
	shared_ptr<CUser>		user;
};