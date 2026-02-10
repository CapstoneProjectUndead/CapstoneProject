#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

#include <ServerEngine/PacketUtils.h>

constexpr int PORT_NUM = 7777;
constexpr int ID_SIZE = 40;
constexpr int PW_SIZE = 40;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

// Packet ID
enum PacketType : uint16_t
{
	_S_PING = 0,
	_C_PONG = 1,
	_C_PING = 2,
	_S_PONG = 3,
	_C_SIGNUP = 4,
	_S_SIGNRES = 5,
	_C_LOGIN = 6,
	_C_LOGOUT = 7,
	_S_LOGIN = 8,
	_S_LOGOUT = 9,
	_C_ROOM_CREATE = 10,
	_C_ROOM_ENTER = 11,
	_S_ROOMLIST = 12,
	_S_SPAWNPLAYER = 13,
	_S_ADDPLAYER = 14,
	_S_PLAYERLIST = 15,
	_S_REMOVEPLAYER = 16,
	_C_PLAYER_INPUT = 17,	// 서버 권위 방식 + 클라 예측 이동
	_S_MOVE = 18,
};

#pragma pack (push, 1)
#include <packet_struct.h>
static_assert(sizeof(PacketHeader) == 4, "PacketHeader size mismatch!");

//=============================
// 서버 RTT 측정
struct S_Ping : public PacketHeader
{
	float server_send_time;

	S_Ping() : PacketHeader(sizeof(S_Ping), (UINT)PacketType::_S_PING) {}
};

struct C_Pong : public PacketHeader
{
	float server_send_time; // 그대로 반사

	C_Pong() : PacketHeader(sizeof(C_Pong), (UINT)PacketType::_C_PONG) {}
};
//=============================

//=============================
// 클라 clock sync (시간 맞추기)
struct C_Ping : public PacketHeader
{
	float clientTime;

	C_Ping() : PacketHeader(sizeof(C_Ping), (UINT)PacketType::_C_PING) {}
};

struct S_Pong : public PacketHeader
{
	float clientTime; // 그대로 반사
	float serverTime; // 서버가 찍은 시간

	S_Pong() : PacketHeader(sizeof(S_Pong), (UINT)PacketType::_S_PONG) {}
};
//=============================

struct C_SIGNUP : public PacketHeader
{
	char	id[ID_SIZE];
	char	password[PW_SIZE];
	char	name[NAME_SIZE];

	C_SIGNUP() : PacketHeader(sizeof(C_SIGNUP), _C_SIGNUP) {}
};
static_assert(sizeof(C_SIGNUP) == 4 + 100, "C_SIGNUP size mismatch!");

struct S_SIGN_RES : public PacketHeader
{
	bool success;

	S_SIGN_RES() : PacketHeader(sizeof(S_SIGN_RES), _S_SIGNRES) {}
};
static_assert(sizeof(S_SIGN_RES) == 4 + 1, "S_SIGN_RES size mismatch!");

struct C_LOGIN : public PacketHeader
{
	char	id[ID_SIZE];
	char	password[PW_SIZE];

	C_LOGIN() : PacketHeader(sizeof(C_LOGIN), (UINT)PacketType::_C_LOGIN) {}
};
static_assert(sizeof(C_LOGIN) == 4 + 80, "C_LOGIN size mismatch!");

struct C_LOGOUT : public PacketHeader
{
	uint64 user_id;

	C_LOGOUT() : PacketHeader(sizeof(C_LOGOUT), (UINT)PacketType::_C_LOGOUT) {}
};
static_assert(sizeof(C_LOGOUT) == 4 + 8, "C_LOGOUT size mismatch!");

struct S_LOGIN : public PacketHeader
{
	uint64	user_id;
	bool	success;

	S_LOGIN() : PacketHeader(sizeof(S_LOGIN), (UINT)PacketType::_S_LOGIN) {}
};
static_assert(sizeof(S_LOGIN) == 4 + 9, "S_LOGIN size mismatch!");

struct S_LOGOUT : public PacketHeader
{
	bool	success;

	S_LOGOUT() : PacketHeader(sizeof(S_LOGOUT), (UINT)PacketType::_S_LOGOUT) {}
};
static_assert(sizeof(S_LOGOUT) == 4 + 1, "S_LOGOUT size mismatch!");

struct C_CreateRoom
{
	uint64 player_id;
	char room_name[ROOM_NAME_MAX];
};

// 내 플레이어를 보낼 떄
struct S_SpawnPlayer : public PacketHeader
{
	NetObjectInfo info;

	S_SpawnPlayer() : PacketHeader(sizeof(S_SpawnPlayer), (UINT)PacketType::_S_SPAWNPLAYER) {}
};
static_assert(sizeof(S_SpawnPlayer) == 4 + 45, "S_SpawnPlayer size mismatch!");

// 한명의 유저를 보낼 때 
struct S_AddPlayer : public PacketHeader
{
	NetObjectInfo info;

	S_AddPlayer() : PacketHeader(sizeof(S_AddPlayer), (UINT)PacketType::_S_ADDPLAYER) {}
};
static_assert(sizeof(S_AddPlayer) == 4 + 45, "S_AddPlayer size mismatch!");

// 가변인자 패킷
// 여러 유저를 패킷에 담아서 보낸다.
struct S_PLAYER_LIST : public PacketHeader
{
	struct Player
	{
		NetObjectInfo info;
		//char	name[NAME_SIZE];

		Player(NetObjectInfo _info)
			: info(_info)
		{ }

		Player(NetObjectInfo _info, const char* _name)
			: info(_info)
		{
			//COPY_STRING(name, _name);
		}
	};

	uint32  buff_offset;
	uint32	player_count;

	S_PLAYER_LIST(int32 count) : PacketHeader(sizeof(S_PLAYER_LIST), (UINT)PacketType::_S_PLAYERLIST) {}

	using PlayerList = PacketList<S_PLAYER_LIST::Player>;

	PlayerList GetPlayerList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return PlayerList(reinterpret_cast<Player*>(data), player_count);
	}
};
static_assert(sizeof(S_PLAYER_LIST) == 4 + 8, "S_PLAYER_LIST size mismatch!");

struct S_RemovePlayer : public PacketHeader
{
	NetObjectInfo info;

	S_RemovePlayer() : PacketHeader(sizeof(S_RemovePlayer), (UINT)PacketType::_S_REMOVEPLAYER) {}
};
static_assert(sizeof(S_RemovePlayer) == 4 + 45, "S_RemovePlayer size mismatch!");

// 서버 권한 + 클라 예측
struct C_Input : public PacketHeader
{
	uint64			seq_num;	// 클라이언트가 자체적으로 1씩 올리는 번호
	float           duration;  // 클라이언트가 이 입력을 유지한 시간
	NetObjectInfo	info;

	C_Input() : PacketHeader(sizeof(C_Input), (UINT)PacketType::_C_PLAYER_INPUT)
		, duration(0.0f)
	{
	};
};
static_assert(sizeof(C_Input) == 4 + 57, "C_PlayerInput size mismatch!");

struct S_Move : public PacketHeader
{
	uint64			last_seq_num;
	float			timestamp;
	NetObjectInfo	info;

	S_Move() : PacketHeader(sizeof(S_Move), (UINT)PacketType::_S_MOVE) {}
};
static_assert(sizeof(S_Move) == 4 + 57, "S_Move size mismatch!");

#pragma pack (pop)