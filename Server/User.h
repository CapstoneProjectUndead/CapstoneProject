#pragma once
// ServerÂÊ User

class CPlayer;
class CRoom;

class CUser
{
public:
	CUser();
	~CUser();

	weak_ptr<Session>    GetSessionWeak() const { return session; }
	shared_ptr<Session>  GetSession() const { return session.lock(); }
	void                 SetSession(shared_ptr<Session> _session) { session = _session; }

	uint64 GetUserID() const { return user_id; }

	const  string& GetName() const { return name; }
	void   SetName(const string& _name) { name = _name; }

	uint32 GetRoomID() const { return room_id; }
	void   SetRoomID(const uint32 id) { room_id = id; }

	shared_ptr<CPlayer> GetPlayer() { return player; }
	void SetPlayer(shared_ptr<CPlayer> _player) { player = _player; }

	CRoom* GetRoom() { return room; }
	void SetRoom(CRoom* _room) { room = _room; }

private:
	static atomic<uint64> s_userid_generator;

	const uint64          user_id;
	string				  name;
	uint32				  room_id;
	weak_ptr<Session>	  session;
	shared_ptr<CPlayer>	  player;
	CRoom*				  room;
};

