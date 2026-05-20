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
	_C_LEAVE_ROOM,
	_C_ENTER_SCENE,
	_S_ENTER_ROOM,
	_S_LEAVE_ROOM,
	_S_ENTER_SCENE,
	_S_ROOM_LIST,
	_S_SPAWN_PLAYER,
	_S_PLAYER_LIST,
	_S_REMOVE_PLAYER,

	_C_PLAYER_INPUT,	// 서버 권위 방식 + 클라 예측 이동
	_S_PLAYER_MOVE,

	_C_CUSTOM_SELECT,
	_S_CUSTOM_SELECT,
	_C_SCENE_CHANGE,
	_S_SCENE_CHANGE,
	_S_SPAWN_MONSTER,
	_S_DESPAWN_MONSTER,
	_S_MONSTER_MOVE,

	_S_MAP_START,
	_S_MAP_DATA,
	_S_MAP_END,

	_C_READY,	// 로비씬에서 사신에게 준비 완료 버튼 누름
	_S_READY,

	_S_SPAWN_ITEM,      // 서버 → 클라: 월드에 보물 생성
	_S_SPAWN_ITEM_LIST,
	_S_DESPAWN_ITEM,
	_C_PICKUP_ITEM,     // 클라 → 서버: 보물 줍기 요청
	_S_ADD_ITEM,		// 서버 → 클라: 인벤토리에 추가해라
	_S_ADD_ITEM_LIST,	// 서버 → 클라: 인벤토리에 여러 아이템 추가해라
	_S_REMOVE_ITEM,		// 서버 → 클라: 인벤토리에서 없애라
	_C_DROP_ITEM,		// 클라 → 서버
	_C_EQUIP_ITEM,
	_S_EQUIP_ITEM,
	_C_USE_ITEM,
	_S_USE_ITEM,

	_S_MINEABLE_LIST,
	_S_DESTROY_MINEABLE,
	_S_UPDATE_DURABILITY,

	_S_PLAY_SOUND,

	_S_POSSESSION_RELEASE_FAIL,

	// 정산 시스템
	_S_RETURN_ZONE_ACTIVE, // 서버 → 클라: 복귀존 활성화 (라운드 종료 60초 전, 1회)
	_S_PLAYER_RETURNED,    // 서버 → 클라: 특정 플레이어가 복귀존 진입 (1회)
	_S_GAME_SETTLEMENT,    // 서버 → 클라: 라운드 종료 정산 결과
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

struct C_LeaveRoom : public PacketHeader
{
	uint64 user_id;

	C_LeaveRoom() : PacketHeader(sizeof(C_LeaveRoom), (UINT)PacketType::_C_LEAVE_ROOM) {}
};
static_assert(sizeof(C_LeaveRoom) == 4 + 8, "C_LeaveRoom size mismatch!");

struct S_EnterRoom : public PacketHeader
{
	bool success;
	uint32 room_id;
	SCENE_TYPE scene_type;

	S_EnterRoom() : PacketHeader(sizeof(S_EnterRoom), (UINT)PacketType::_S_ENTER_ROOM) {}
};
static_assert(sizeof(S_EnterRoom) == 4 + 6, "S_EnterRoom size mismatch!");

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
static_assert(sizeof(S_SpawnPlayer) == 4 + 70, "S_SpawnPlayer size mismatch!");

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
static_assert(sizeof(S_PLAYER_LIST) == 4 + 13, "S_PLAYER_LIST size mismatch!");

struct S_RemovePlayer : public PacketHeader
{
	uint64	   player_id;
	SCENE_TYPE scene_type;

	S_RemovePlayer() : PacketHeader(sizeof(S_RemovePlayer), (UINT)PacketType::_S_REMOVE_PLAYER) {}
};
static_assert(sizeof(S_RemovePlayer) == 4 + 9, "S_RemovePlayer size mismatch!");

// 서버 권한 + 클라 예측
struct C_Input : public PacketHeader
{
	uint64			seq_num;	// 클라이언트가 자체적으로 1씩 올리는 번호
	NetPlayerInfo	info;
	uint32			room_id;
	SCENE_TYPE		scene_type;
	float           duration;    // 클라이언트가 이 입력을 유지한 시간

	C_Input() : PacketHeader(sizeof(C_Input), (UINT)PacketType::_C_PLAYER_INPUT)
		, duration(0.0f)
	{
	};
};
static_assert(sizeof(C_Input) == 4 + 81, "C_PlayerInput size mismatch!");

struct S_PlayerMove : public PacketHeader
{
	uint64			last_seq_num;
	float			timestamp;
	NetPlayerInfo	info;
	SCENE_TYPE		scene_type;
	uint32			stamina;
	uint32			hp;
	float			round_timer;  // 게임씬일 때만 유효 (>=0). 그 외 -1

	S_PlayerMove() : PacketHeader(sizeof(S_PlayerMove), (UINT)PacketType::_S_PLAYER_MOVE)
		, round_timer(-1.f)
	{}
};
static_assert(sizeof(S_PlayerMove) == 4 + 89, "S_PlayerMove size mismatch!");

struct C_CustomSelect : public PacketHeader
{
	uint64 player_id;
	uint8  body_type;
	uint8  eye_type;
	uint8  mouth_type;

	C_CustomSelect() : PacketHeader(sizeof(C_CustomSelect), (UINT)PacketType::_C_CUSTOM_SELECT) {}
};
static_assert(sizeof(C_CustomSelect) == 4 + 11, "C_CustomSelect size mismatch!");

struct S_CustomSelect : public PacketHeader
{
	S_CustomSelect() : PacketHeader(sizeof(S_CustomSelect), (UINT)PacketType::_S_CUSTOM_SELECT) {}
};
static_assert(sizeof(S_CustomSelect) == 4, "S_CustomSelect size mismatch!");

struct C_SceneChange : public PacketHeader
{
	uint64     player_id;
	SCENE_TYPE current_scene;
	SCENE_TYPE target_scene;

	C_SceneChange() : PacketHeader(sizeof(C_SceneChange), (UINT)PacketType::_C_SCENE_CHANGE) {}
};
static_assert(sizeof(C_SceneChange) == 4 + 10, "C_SceneChange size mismatch!");

struct S_SceneChange : public PacketHeader
{
	uint64     player_id;
	SCENE_TYPE current_scene;
	SCENE_TYPE target_scene;

	S_SceneChange() : PacketHeader(sizeof(S_SceneChange), (UINT)PacketType::_S_SCENE_CHANGE) {}
};
static_assert(sizeof(S_SceneChange) == 4 + 10, "S_SceneChange size mismatch!");

struct S_SpawnMonster : public PacketHeader
{
	NetMonsterInfo info;    
	uint32         room_id;
	SCENE_TYPE     scene_type;

	S_SpawnMonster() : PacketHeader(sizeof(S_SpawnMonster), (UINT)PacketType::_S_SPAWN_MONSTER) {}
};
static_assert(sizeof(S_SpawnMonster) == 4 + 58, "S_SpawnMonster size mismatch!");

struct S_DeSpawnMonster : public PacketHeader
{
	uint64		   monster_id;
	uint32         room_id;
	SCENE_TYPE     scene_type;

	S_DeSpawnMonster() : PacketHeader(sizeof(S_DeSpawnMonster), (UINT)PacketType::_S_DESPAWN_MONSTER) {}
};
static_assert(sizeof(S_DeSpawnMonster) == 4 + 13, "S_DeSpawnMonster size mismatch!");

struct S_MonsterMove : public PacketHeader
{
	float			timestamp;
	NetMonsterInfo	info;
	SCENE_TYPE		scene_type;

	S_MonsterMove() : PacketHeader(sizeof(S_MonsterMove), (UINT)PacketType::_S_MONSTER_MOVE) {}
};
static_assert(sizeof(S_MonsterMove) == 4 + 58, "S_MonsterMove size mismatch!");

struct S_MapStart : public PacketHeader
{
	S_MapStart() : PacketHeader(sizeof(S_MapStart), _S_MAP_START) {}
};
static_assert(sizeof(S_MapStart) == 4, "S_MapStart size mismatch!");

struct S_MapData : public PacketHeader
{
	//uint16    chunk_index;  // 몇 번째 조각인지 (0, 1, 2...)
	//uint16    total_chunks; // 총 몇 조각인지 (82개 등)

	uint16    data_count;    // 이번 패킷에 담긴 구조체 개수
	NetPacket::InstanceData data[60];   // 17바이트 구조체 x 60

	S_MapData() : PacketHeader(sizeof(S_MapData), _S_MAP_DATA) {}
};
static_assert(sizeof(S_MapData) == 4 + 1142, "S_MapData size mismatch!");

struct S_MapEnd : public PacketHeader
{
	S_MapEnd() : PacketHeader(sizeof(S_MapEnd), _S_MAP_END) {}
};
static_assert(sizeof(S_MapEnd) == 4, "S_MapEnd size mismatch!");

// 서버 → 클라: CMineableObject 목록 (world_id + 위치). S_MapEnd 직후 송신
struct S_MineableList : public PacketHeader
{
	struct Mineable
	{
		uint32 world_id;
		float  x, y, z;
		MINEABLEOBJECT_TYPE type;
	}; 

	uint32      buff_offset;
	uint32      mineable_count;
	SCENE_TYPE  scene_type;

	S_MineableList() : PacketHeader(sizeof(S_MineableList), (UINT)PacketType::_S_MINEABLE_LIST) {}

	using MineableList = PacketList<S_MineableList::Mineable>;

	MineableList GetMineableList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return MineableList(reinterpret_cast<Mineable*>(data), mineable_count);
	}
};
static_assert(sizeof(S_MineableList) == 4 + 9, "S_MineableList size mismatch!");

struct C_Ready : public PacketHeader
{
	uint64 player_id;

	C_Ready() : PacketHeader(sizeof(C_Ready), _C_READY) {}
};
static_assert(sizeof(C_Ready) == 4 + 8, "C_Ready size mismatch!");

struct S_Ready : public PacketHeader
{
	uint64 player_id;

	S_Ready() : PacketHeader(sizeof(S_Ready), _S_READY) {}
};
static_assert(sizeof(S_Ready) == 4 + 8, "S_Ready size mismatch!");

// 서버 → 클라: 월드에 보물 생성 (위치 + 고유 ID)
struct S_SpawnItem : public PacketHeader
{
	uint16 item_id;        // 아이템 도감 번호
	uint32 item_world_id;  // 오브젝트 고유 ID
	ITEM_TYPE item_type;
	SCENE_TYPE scene_type;
	float  x, y, z;

	S_SpawnItem() : PacketHeader(sizeof(S_SpawnItem), (UINT)PacketType::_S_SPAWN_ITEM) {}
};
static_assert(sizeof(S_SpawnItem) == 4 + 20, "S_SpawnItem size mismatch!");

struct S_Spawn_Item_List : public PacketHeader
{
	struct Item
	{
		ITEM_TYPE   item_type;
		uint16		item_id;
		uint32		item_world_id;
		float		x, y, z;
	};

	uint32		buff_offset;
	uint32      item_count;
	SCENE_TYPE	scene_type;

	S_Spawn_Item_List(int32 count) : PacketHeader(sizeof(S_Spawn_Item_List), (UINT)PacketType::_S_SPAWN_ITEM_LIST) {}

	using ItemList = PacketList<S_Spawn_Item_List::Item>;

	ItemList GetItemList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return ItemList(reinterpret_cast<Item*>(data), item_count);
	}
};
static_assert(sizeof(S_Spawn_Item_List) == 4 + 9, "S_Spawn_Item_List size mismatch!");

struct S_DeSpawnItem : public PacketHeader
{
	uint32 item_world_id;  // 오브젝트 고유 ID
	ITEM_TYPE item_type;
	SCENE_TYPE scene_type;

	S_DeSpawnItem() : PacketHeader(sizeof(S_DeSpawnItem), (UINT)PacketType::_S_DESPAWN_ITEM) {}
};
static_assert(sizeof(S_DeSpawnItem) == 4 + 6, "S_DeSpawnItem size mismatch!");

// 클라 → 서버: 보물 줍기 요청
struct C_PickupItem : public PacketHeader
{
	uint64 player_id;
	uint32 item_world_id; 
	ITEM_TYPE item_type;
	SCENE_TYPE scene_type;

	C_PickupItem() : PacketHeader(sizeof(C_PickupItem), (UINT)PacketType::_C_PICKUP_ITEM) {}
};
static_assert(sizeof(C_PickupItem) == 4 + 14, "C_PickupItem size mismatch!");

struct S_AddItem : public PacketHeader
{
	uint64 player_id;
	uint16 item_id;
	uint32 item_world_id;
	uint32 inventory_id;
	ITEM_TYPE item_type;
	SCENE_TYPE scene_type;

	S_AddItem() : PacketHeader(sizeof(S_AddItem), (UINT)PacketType::_S_ADD_ITEM) {}
};
static_assert(sizeof(S_AddItem) == 4 + 20, "S_AddItem size mismatch!");

struct S_AddItemList : public PacketHeader
{
	struct Item
	{
		uint16 item_id;
		uint32 inventory_id;
		ITEM_TYPE item_type;
	};

	uint64 player_id;
	SCENE_TYPE scene_type;
	uint16 buff_offset;
	uint16 item_count;

	using ItemList = PacketList<S_AddItemList::Item>;

	ItemList GetItemList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return ItemList(reinterpret_cast<Item*>(data), item_count);
	}

	S_AddItemList() : PacketHeader(sizeof(S_AddItemList), (UINT)PacketType::_S_ADD_ITEM_LIST) {}
};
static_assert(sizeof(S_AddItemList) == 4 + 13, "S_AddItemList size mismatch!");

struct S_RemoveItem : public PacketHeader
{
	uint64 player_id;
	uint32 inventory_id;
	uint16 item_id;
	SCENE_TYPE scene_type;

	S_RemoveItem() : PacketHeader(sizeof(S_RemoveItem), (UINT)PacketType::_S_REMOVE_ITEM) {}
};
static_assert(sizeof(S_RemoveItem) == 4 + 15, "S_RemoveItem size mismatch!");

struct C_DropItem : public PacketHeader
{
	uint64 player_id;
	uint32 inventory_id;
	ITEM_TYPE item_type;
	SCENE_TYPE scene_type;

	C_DropItem() : PacketHeader(sizeof(C_DropItem), (UINT)PacketType::_C_DROP_ITEM) {}
};
static_assert(sizeof(C_DropItem) == 4 + 14, "C_DropItem size mismatch!");

struct C_EquipItem : public PacketHeader
{
	bool   is_dowsing_rod = false;
	uint64 player_id;
	uint32 inventory_id; 
	int16 item_id;	// 아이템 도감번호
	SCENE_TYPE scene_type;

	C_EquipItem() : PacketHeader(sizeof(C_EquipItem), (UINT)PacketType::_C_EQUIP_ITEM) {}
};
static_assert(sizeof(C_EquipItem) == 4 + 16, "C_EquipItem size mismatch!");

struct S_EquipItem : public PacketHeader
{
	bool   is_dowsing_rod = false;
	uint64 player_id;
	int16 item_id;	// 아이템 도감번호
	SCENE_TYPE scene_type;

	S_EquipItem() : PacketHeader(sizeof(S_EquipItem), (UINT)PacketType::_S_EQUIP_ITEM) {}
};
static_assert(sizeof(S_EquipItem) == 4 + 12, "S_EquipItem size mismatch!");

struct C_UseItem : public PacketHeader
{
	uint64 player_id;
	SCENE_TYPE scene_type;
	uint16 item_id;
	uint32 inventory_id;

	C_UseItem() : PacketHeader(sizeof(C_UseItem), (UINT)PacketType::_C_USE_ITEM) {}
};
static_assert(sizeof(C_UseItem) == 4 + 15, "C_UseItem size mismatch!");

struct S_UseItem : public PacketHeader
{
	uint64 player_id;
	SCENE_TYPE scene_type;
	bool success;

	S_UseItem() : PacketHeader(sizeof(S_UseItem), (UINT)PacketType::_S_USE_ITEM) {}
};
static_assert(sizeof(S_UseItem) == 4 + 10, "S_UseItem size mismatch!");

struct S_DestroyMineable : public PacketHeader
{
	uint64 obj_id;
	SCENE_TYPE scene_type;

	S_DestroyMineable() : PacketHeader(sizeof(S_DestroyMineable), (UINT)PacketType::_S_DESTROY_MINEABLE) {}
};
static_assert(sizeof(S_DestroyMineable) == 4 + 9, "S_DestroyMineable size mismatch");

struct S_UpdateDurability : public PacketHeader
{
	uint64 player_id;
	uint16 item_id;
	uint32 inventory_id;
	uint32 current_durability;
	ITEM_TYPE item_type;
	ITEM_SUB_TYPE item_sub_type;
	SCENE_TYPE scene_type;

	S_UpdateDurability() : PacketHeader(sizeof(S_UpdateDurability), (UINT)PacketType::_S_UPDATE_DURABILITY) {}
};
static_assert(sizeof(S_UpdateDurability) == 4 + 22, "S_UpdateDurability size mismatch!");

struct S_PlaySound : public PacketHeader
{
	bool is_global = false;
	float range = -1.f;
	float x, y, z;
	SOUND_ID sound_id;
	int64 player_id = -1;
	SCENE_TYPE scene_type;

	S_PlaySound() : PacketHeader(sizeof(S_PlaySound), (UINT)PacketType::_S_PLAY_SOUND) {}
};
static_assert(sizeof(S_PlaySound) == 4 + 28, "S_PlaySound size mismatch!");

struct S_PossessionReleaseFail : public PacketHeader
{
	uint64 player_id = -1;

	S_PossessionReleaseFail() : PacketHeader(sizeof(S_PlaySound), (UINT)PacketType::_S_POSSESSION_RELEASE_FAIL) {}
};
static_assert(sizeof(S_PossessionReleaseFail) == 4 + 8, "S_PossessionReleaseFail size mismatch!");

// 서버 → 클라: 복귀존 활성화 (라운드 종료 60초 전, 1회 브로드캐스트)
// 확장 대비: 위치/반경을 패킷으로 전달 (지금은 spawn point 고정이지만, 추후 랜덤 지점 변경 가능)
struct S_ReturnZoneActive : public PacketHeader
{
	float      x, y, z;   // 복귀존 중심
	float      range;     // 반경
	SCENE_TYPE scene_type;

	S_ReturnZoneActive() : PacketHeader(sizeof(S_ReturnZoneActive), (UINT)PacketType::_S_RETURN_ZONE_ACTIVE) {}
};
static_assert(sizeof(S_ReturnZoneActive) == 4 + 17, "S_ReturnZoneActive size mismatch!");

// 서버 → 클라: 특정 플레이어가 복귀존 진입 (1회 브로드캐스트)
struct S_PlayerReturned : public PacketHeader
{
	uint64     player_id;
	SCENE_TYPE scene_type;

	S_PlayerReturned() : PacketHeader(sizeof(S_PlayerReturned), (UINT)PacketType::_S_PLAYER_RETURNED) {}
};
static_assert(sizeof(S_PlayerReturned) == 4 + 9, "S_PlayerReturned size mismatch!");

// 서버 → 클라: 라운드 종료 정산 결과 (가변길이, unicast per player)
// 복귀자 100%, 미복귀자 50%, 전원 복귀 시 ×2 보너스
// TreasureEntry는 서버가 item_id별로 묶어서 전송 (검증값)
struct S_GameSettlement : public PacketHeader
{
	struct TreasureEntry
	{
		uint16 item_id;
		uint32 price;    // 개당 가격 (서버 검증값)
		uint16 count;
	};  // 8 bytes

	uint32     base_coin;           // 보물 합산
	uint32     final_coin;          // 보너스/복귀 적용 후 최종
	bool       is_returned;
	bool       all_returned_bonus;
	SCENE_TYPE scene_type;
	uint16     buff_offset;
	uint16     treasure_count;

	S_GameSettlement() : PacketHeader(sizeof(S_GameSettlement), (UINT)PacketType::_S_GAME_SETTLEMENT) {}

	using TreasureList = PacketList<S_GameSettlement::TreasureEntry>;
	TreasureList GetTreasureList()
	{
		BYTE* data = reinterpret_cast<BYTE*>(this);
		data += buff_offset;
		return TreasureList(reinterpret_cast<TreasureEntry*>(data), treasure_count);
	}
};
static_assert(sizeof(S_GameSettlement) == 4 + 15, "S_GameSettlement size mismatch!");

#pragma pack (pop)