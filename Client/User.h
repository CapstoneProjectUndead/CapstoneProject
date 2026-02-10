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

	uint64 GetUserID() const { return user_id; }
	void SetUserID(const uint64 id) { user_id = id; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

private:
	uint64 user_id;
	uint32 room_id;
	std::weak_ptr<Session>     session;
	std::shared_ptr<CMyPlayer> my_player;
};

