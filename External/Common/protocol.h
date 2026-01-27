#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

#include <ServerEngine/PacketUtils.h>

constexpr int PORT_NUM = 7777;
constexpr int ID_SIZE = 30;
constexpr int PW_SIZE = 30;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

// Packet ID
enum PacketType : uint16_t
{
	_C_SIGNUP = 0,
	_S_SIGNRES = 1,
	_C_LOGIN = 2,
	_S_LOGIN = 3,
	_S_LOGIN_FAIL = 4,
	_S_SPAWNPLAYER = 5,
	_S_ADDPLAYER = 6,
	_S_PLAYERLIST = 7,
	_S_REMOVEPLAYER = 8,
	_C_MOVE = 9,
	_S_MOVE = 10,

	// 서버 권한 + 클라 예측 (테스트)
	_C_PLAYER_INPUT = 11,
};

#pragma pack (push, 1)
#include <packet_struct.h>
static_assert(sizeof(PacketHeader) == 4, "PacketHeader size mismatch!");

struct C_LOGIN : public PacketHeader
{
	//char	id[ID_SIZE];
	//char	password[PW_SIZE];

	C_LOGIN() : PacketHeader(sizeof(C_LOGIN), (UINT)PacketType::_C_LOGIN) {}
};
static_assert(sizeof(C_LOGIN) == 4, "C_LOGIN size mismatch!");

struct S_LOGIN : public PacketHeader
{
	uint64	id;
	bool	success;
	//char	name[NAME_SIZE];

	S_LOGIN() : PacketHeader(sizeof(S_LOGIN), (UINT)PacketType::_S_LOGIN) {}
};
static_assert(sizeof(S_LOGIN) == 4 + 9, "S_LOGIN size mismatch!");

// 내 플레이어를 보낼 떄
struct S_SpawnPlayer : public PacketHeader
{
	NetObjectInfo info;

	S_SpawnPlayer() : PacketHeader(sizeof(S_SpawnPlayer), (UINT)PacketType::_S_SPAWNPLAYER) {}
};
static_assert(sizeof(S_SpawnPlayer) == 4 + 33, "S_SpawnPlayer size mismatch!");

// 한명의 유저를 보낼 때 
struct S_AddPlayer : public PacketHeader
{
	NetObjectInfo info;

	S_AddPlayer() : PacketHeader(sizeof(S_AddPlayer), (UINT)PacketType::_S_ADDPLAYER) {}
};
static_assert(sizeof(S_AddPlayer) == 4 + 33, "S_AddPlayer size mismatch!");

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
static_assert(sizeof(S_RemovePlayer) == 4 + 33, "S_RemovePlayer size mismatch!");

struct C_Move : public PacketHeader
{
	NetObjectInfo info;

	C_Move() : PacketHeader(sizeof(C_Move), (UINT)PacketType::_C_MOVE) {}
};
static_assert(sizeof(C_Move) == 4 + 33, "C_Move size mismatch!");

struct S_Move : public PacketHeader
{
	uint64_t		last_seq_num;
	NetObjectInfo	info;

	S_Move() : PacketHeader(sizeof(S_Move), (UINT)PacketType::_S_MOVE) {}
};
static_assert(sizeof(S_Move) == 4 + 41, "S_Move size mismatch!");

// 서버 권한 + 클라 예측
struct C_Input : public PacketHeader
{
	uint64			seq_num;	// 클라이언트가 자체적으로 1씩 올리는 번호
	float           deltaTime;  // 클라이언트가 이 입력을 유지한 시간
	NetObjectInfo	info;

	C_Input() : PacketHeader(sizeof(C_Input), (UINT)PacketType::_C_PLAYER_INPUT) 
		, deltaTime(0.0f)
	{};
};
static_assert(sizeof(C_Input) == 4 + 45, "C_PlayerInput size mismatch!");

#pragma pack (pop)