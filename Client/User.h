#pragma once

class CMyPlayer;

class CUser
{
public:
	CUser();
	~CUser();

	std::weak_ptr<Session>      GetSessionWeak() const { return session; }
	std::shared_ptr<Session>    GetSession() const { return session.lock(); }
	void						SetSession(std::shared_ptr<Session> _session) { session = _session; }

	uint64 GetID() const { return user_id; }
	void SetID(const uint64 id) { user_id = id; }

private:
	uint64 user_id;
	std::weak_ptr<Session>     session;
	std::shared_ptr<CMyPlayer> my_player;
};

