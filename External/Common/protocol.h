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
	_DUMMY = 0,
	_S_PING,
	_C_PONG,
	_C_PING,
	_S_PONG,
	_C_SIGNUP,
	_S_SIGNRES,
	_C_LOGIN,
	_C_LOGOUT,
	_S_LOGIN,
	_S_LOGOUT,
	_C_CREATE_ROOM,
	_C_UPDATE_ROOM,
	_C_ENTER_ROOM,
	_C_ENTER_SCENE,
	_S_CREATE_ROOM,
	_S_ENTER_ROOM,
	_S_ENTER_SCENE,
	_S_ROOM_LIST,
	_S_SPAWN_PLAYER,
	_S_PLAYER_LIST,
	_S_REMOVE_PLAYER,
	_C_PLAYER_INPUT,	// 서버 권위 방식 + 클라 예측 이동
	_S_MOVE,
};

#pragma pack (push, 1)
#include <packet_struct.h>
static_assert(sizeof(PacketHeader) == 4, "PacketHeader size mismatch!");

struct PktDummy : public PacketHeader
{
	uint64 value;

	PktDummy() : PacketHeader(sizeof(PktDummy), (UINT)PacketType::_DUMMY) {}
};
static_assert(sizeof(PktDummy) == 4 + 8, "PktDummy size mismatch!");

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
	char	name[NAME_SIZE];

	S_LOGIN() : PacketHeader(sizeof(S_LOGIN), (UINT)PacketType::_S_LOGIN) {}
};
static_assert(sizeof(S_LOGIN) == 4 + 29, "S_LOGIN size mismatch!");

struct S_LOGOUT : public PacketHeader
{
	bool	success;

	S_LOGOUT() : PacketHeader(sizeof(S_LOGOUT), (UINT)PacketType::_S_LOGOUT) {}
};
static_assert(sizeof(S_LOGOUT) == 4 + 1, "S_LOGOUT size mismatch!");

struct C_CreateRoom : public PacketHeader
{
	uint64 user_id;
	char   room_name[ROOM_NAME_MAX];

	C_CreateRoom() : PacketHeader(sizeof(C_CreateRoom), (UINT)PacketType::_C_CREATE_ROOM) {}
};
static_assert(sizeof(C_CreateRoom) == 4 + 58, "C_CreateRoom size mismatch!");

struct C_UpdateRoom : public PacketHeader
{
	C_UpdateRoom() : PacketHeader(sizeof(C_UpdateRoom), (UINT)PacketType::_C_UPDATE_ROOM) {}
};
static_assert(sizeof(C_UpdateRoom) == 4, "C_UpdateRoom size mismatch!");

struct C_EnterRoom : public PacketHeader
{
	uint64 user_id;
	uint32 room_id;

	C_EnterRoom() : PacketHeader(sizeof(C_EnterRoom), (UINT)PacketType::_C_ENTER_ROOM) {}
};
static_assert(sizeof(C_EnterRoom) == 4 + 12, "C_EnterRoom size mismatch!");

// S_CreateRoom 패킷은 필요가 없는 것 같다...
struct S_CreateRoom : public PacketHeader
{
	NetRoomInfo room_info;

	S_CreateRoom() : PacketHeader(sizeof(S_CreateRoom), (UINT)PacketType::_S_CREATE_ROOM) {}
};
static_assert(sizeof(S_CreateRoom) == 4 + 57, "S_CreateRoom size mismatch!");

struct S_EnterRoom : public PacketHeader
{
	bool success;
	uint32 room_id;
	SCENE_TYPE scene_type;

	S_EnterRoom() : PacketHeader(sizeof(S_EnterRoom), (UINT)PacketType::_S_ENTER_ROOM) {}
};
static_assert(sizeof(S_EnterRoom) == 4 + 9, "S_EnterRoom size mismatch!");

// 가변길이 패킷
// 여러 방 정보를 패킷에 담아서 보낸다.
struct S_Room_List : public PacketHeader
{
	struct Room
	{
		NetRoomInfo room_info;
	};

	uint16  buff_offset;
	uint16	room_count;

	S_Room_List() : PacketHeader(sizeof(S_Room_List), (UINT)PacketType::_S_ROOM_LIST) {}

	using RoomList = PacketList<S_Room_List::Room>;

	RoomList GetRoomList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return RoomList(reinterpret_cast<Room*>(data), room_count);
	}
};
static_assert(sizeof(S_Room_List) == 4 + 4, "S_Room_List size mismatch!");

// 내 플레이어 또는 상대 플레이어를 보낼 때
struct S_SpawnPlayer : public PacketHeader
{
	bool is_my_player;		// 아래 NetPlayerInfo 구조체에도 있지만, 까먹을까봐 여기서 처리한다. 
	NetPlayerInfo info;
	uint32     room_id;
	SCENE_TYPE scene_type;

	S_SpawnPlayer() : PacketHeader(sizeof(S_SpawnPlayer), (UINT)PacketType::_S_SPAWN_PLAYER) {}
};
static_assert(sizeof(S_SpawnPlayer) == 4 + 63, "S_SpawnPlayer size mismatch!");

// 가변인자 패킷
// 여러 유저를 패킷에 담아서 보낸다.
struct S_PLAYER_LIST : public PacketHeader
{
	struct Player
	{
		NetPlayerInfo info;
		//char	name[NAME_SIZE];

		Player(NetPlayerInfo _info)
			: info(_info)
		{ }

		Player(NetPlayerInfo _info, const char* _name)
			: info(_info)
		{
			//COPY_STRING(name, _name);
		}
	};

	uint32		buff_offset;
	uint32		player_count;
	uint32		room_id;
	SCENE_TYPE	scene_type;

	S_PLAYER_LIST(int32 count) : PacketHeader(sizeof(S_PLAYER_LIST), (UINT)PacketType::_S_PLAYER_LIST) {}

	using PlayerList = PacketList<S_PLAYER_LIST::Player>;

	PlayerList GetPlayerList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return PlayerList(reinterpret_cast<Player*>(data), player_count);
	}
};
static_assert(sizeof(S_PLAYER_LIST) == 4 + 16, "S_PLAYER_LIST size mismatch!");

struct S_RemovePlayer : public PacketHeader
{
	NetPlayerInfo info;
	SCENE_TYPE scene_type;

	S_RemovePlayer() : PacketHeader(sizeof(S_RemovePlayer), (UINT)PacketType::_S_REMOVE_PLAYER) {}
};
static_assert(sizeof(S_RemovePlayer) == 4 + 58, "S_RemovePlayer size mismatch!");

// 서버 권한 + 클라 예측
struct C_Input : public PacketHeader
{
	uint64			seq_num;	// 클라이언트가 자체적으로 1씩 올리는 번호
	NetPlayerInfo	info;
	uint32			room_id;
	SCENE_TYPE		scene_type;
	float           duration;   // 클라이언트가 이 입력을 유지한 시간

	C_Input() : PacketHeader(sizeof(C_Input), (UINT)PacketType::_C_PLAYER_INPUT)
		, duration(0.0f)
	{
	};
};
static_assert(sizeof(C_Input) == 4 + 74, "C_PlayerInput size mismatch!");

struct S_Move : public PacketHeader
{
	uint64			last_seq_num;
	float			timestamp;
	NetPlayerInfo	info;
	SCENE_TYPE		scene_type;

	S_Move() : PacketHeader(sizeof(S_Move), (UINT)PacketType::_S_MOVE) {}
};
static_assert(sizeof(S_Move) == 4 + 70, "S_Move size mismatch!");

#pragma pack (pop)