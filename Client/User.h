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

	const  std::string & GetName() const { return name; }
	void   SetName(const std::string& _name) { name = _name; }

	uint32 GetRoomID() const { return room_id; }
	void SetRoomID(const uint32 id) { room_id = id; }

	std::shared_ptr<CMyPlayer> GetMyPlayer() const { return my_player; }
	void SetMyPlayer(std::shared_ptr<CMyPlayer> player) { my_player = player; }

private:
	uint64					   user_id;
	std::string				   name;
	uint32					   room_id;
	std::weak_ptr<Session>     session;
	std::shared_ptr<CMyPlayer> my_player;
};

