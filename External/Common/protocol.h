#pragma once
#include <ServerEngine/PacketUtils.h>

constexpr int PORT_NUM = 7777;
constexpr int ID_SIZE = 30;
constexpr int PW_SIZE = 30;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

// Packet ID
constexpr int16 _C_SIGNUP = 0;
constexpr int16 _S_SIGNRES = 1;
constexpr int16 _C_LOGIN = 2;
constexpr int16 _S_LOGIN = 3;
constexpr int16 _S_LOGIN_FAIL = 4;
constexpr int16 _S_SPAWNPLAYER = 5;
constexpr int16 _S_ADDPLAYER = 6;
constexpr int16 _S_PLAYERLIST = 7;
constexpr int16 _S_REMOVEPLAYER = 8;
constexpr int16 _C_MOVE = 9;
constexpr int16 _S_MOVE = 10;

// 서버 권한 + 클라 예측 (테스트)
constexpr int16 _C_PLAYER_INPUT = 11;

#pragma pack (push, 1)

static_assert(sizeof(PacketHeader) == 4, "PacketHeader size mismatch!");

struct C_LOGIN : public PacketHeader
{
	//char	id[ID_SIZE];
	//char	password[PW_SIZE];

	C_LOGIN() : PacketHeader(sizeof(C_LOGIN), _C_LOGIN) {}
};

struct S_LOGIN : public PacketHeader
{
	uint64	id;
	bool	success;
	//char	name[NAME_SIZE];

	S_LOGIN() : PacketHeader(sizeof(S_LOGIN), _S_LOGIN) {}
};

// 내 플레이어를 보낼 떄
struct S_SpawnPlayer : public PacketHeader
{
	ObjectInfo info;

	S_SpawnPlayer() : PacketHeader(sizeof(S_SpawnPlayer), _S_SPAWNPLAYER) {}
};

// 한명의 유저를 보낼 때 
struct S_AddPlayer : public PacketHeader
{
	ObjectInfo info;

	S_AddPlayer() : PacketHeader(sizeof(S_AddPlayer), _S_ADDPLAYER) {}
};

// 가변인자 패킷
// 여러 유저를 패킷에 담아서 보낸다.
struct S_PLAYER_LIST : public PacketHeader
{
	struct Player
	{
		ObjectInfo info;
		//char	name[NAME_SIZE];

		Player(ObjectInfo _info)
			: info(_info)
		{ }

		Player(ObjectInfo _info, const char* _name)
			: info(_info)
		{
			//COPY_STRING(name, _name);
		}
	};

	uint32  buff_offset;
	uint32	player_count;

	S_PLAYER_LIST(int32 count) : PacketHeader(sizeof(S_PLAYER_LIST), _S_PLAYERLIST) {}

	using PlayerList = PacketList<S_PLAYER_LIST::Player>;

	PlayerList GetPlayerList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return PlayerList(reinterpret_cast<Player*>(data), player_count);
	}
};

struct S_RemovePlayer : public PacketHeader
{
	ObjectInfo info;

	S_RemovePlayer() : PacketHeader(sizeof(S_RemovePlayer), _S_REMOVEPLAYER) {}
};

struct C_Move : public PacketHeader
{
	ObjectInfo info;

	C_Move() : PacketHeader(sizeof(C_Move), _C_MOVE) {}
};

struct S_Move : public PacketHeader
{
	ObjectInfo info;

	S_Move() : PacketHeader(sizeof(S_Move), _S_MOVE) {}
};

// 서버 권한 + 클라 예측
struct C_PlayerInput : public PacketHeader
{
	int		playerId;
	bool	w, a, s, d;
	float	mouseDeltaX, mouseDeltaY;

	C_PlayerInput() : PacketHeader(sizeof(C_PlayerInput), _C_PLAYER_INPUT) {};
};

#pragma pack (pop)