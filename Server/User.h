#pragma once
// ServerÂÊ User

class CPlayer;

class CUser
{
public:
	CUser();
	~CUser();

	weak_ptr<Session>    GetSessionWeak() const { return session; }
	shared_ptr<Session>  GetSession() const { return session.lock(); }
	void                 SetSession(shared_ptr<Session> _session) { session = _session; }

	uint64 GetID() const { return user_id; }
	void SetID(const uint64 id) { user_id = id; }

	shared_ptr<CPlayer> GetPlayer() { return player; }
	void SetPlayer(shared_ptr<CPlayer> _player) { player = _player; }

private:
	static atomic<uint64> s_idGenerator;

	uint64                user_id;
	weak_ptr<Session>	  session;
	shared_ptr<CPlayer>	  player;
};

