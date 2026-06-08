#pragma once
// Server쪽 User

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

	void SetGuest(bool g) { is_guest = g; }
	bool GetIsGuest() const { return is_guest; }

	const  string& GetName() const { return name; }
	void   SetName(const string& _name) { name = _name; }

	// 로그인 계정 ID (DB users.id / users.json 키). 게스트는 비어있음. 저장/로드 키로 사용.
	const  string& GetAccountId() const { return account_id; }
	void   SetAccountId(const string& _id) { account_id = _id; }

	uint32 GetRoomID() const { return room_id; }
	void   SetRoomID(const uint32 id) { room_id = id; }

	shared_ptr<CPlayer> GetPlayer() { return player; }
	void SetPlayer(shared_ptr<CPlayer> _player) { player = _player; }

	shared_ptr<CRoom> GetRoom() { return room; }
	void SetRoom(shared_ptr<CRoom> _room) { room = _room; }

private:
	static atomic<uint64> s_userid_generator;

	bool				  is_guest;
	const uint64          user_id;
	string				  name;
	string				  account_id;
	uint32				  room_id;
	weak_ptr<Session>	  session;
	shared_ptr<CPlayer>	  player;
	shared_ptr<CRoom>	  room;
};

