#pragma once
#include <ServerEngine\Session.h>

class CPlayer;

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
	shared_ptr<CPlayer> GetPlayer() { return player; }
	void SetPlayer(shared_ptr<CPlayer> pl) { player = pl; }

private:
	shared_ptr<CPlayer>		player;
};

