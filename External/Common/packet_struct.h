#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

// 네트워크 패킷 전용 구조체 선언하는 헤더
// 네이밍 규칙 : Net + 구조체 이름 

// **** 패킷에 포함할 구조체만 선언! ****

//struct NetInputData
//{
//	// 이동 관련 (서버가 검증)
//	bool w = false;
//	bool a = false;
//	bool s = false;
//	bool d = false;
//};

struct NetPlayerInfo
{
	uint64			player_id;
	uint32			room_id;
	bool			is_my_player;

	// 커스터마이징 추가 (26. 2. 25)
	uint8           body_type = 0;
	uint8           eyes_type = 0;
	uint8           mouth_type = 0;

	// 서버권위 방식) w,a,s,d 서버권위 방식에서 필요한 데이터이다.
	bool			w = false;
	bool			a = false;
	bool			s = false;
	bool			d = false;
	bool			space = false;
	bool			shift = false;
	bool            lbtn  = false;
	bool            is_grounded = true;

	float			x, y, z;
	float			vx		= 0.0f;
	float			vy		= 0.0f;
	float			vz		= 0.0f;
	float			yaw		= 0.0f;
	float			pitch	= 0.0f;
	float			roll	= 0.0f; 

	PLAYER_STATE	state;
	bool			is_possessed = false;

	NetPlayerInfo() = default;
	NetPlayerInfo(uint64 _id, uint32 roomId, uint8 bodyType, uint8 eyeType, uint8 mouthType 
		, float _x, float _y, float _z)
		: player_id(_id)
		, room_id(roomId)
		, is_my_player(false)
		, body_type(bodyType)
		, eyes_type(eyeType)
		, mouth_type(mouthType)
		, state(PLAYER_STATE::IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{ }

	NetPlayerInfo(const NetPlayerInfo& other)
		: player_id(other.player_id)
		, room_id(other.room_id)
		, is_my_player(other.is_my_player)
		, body_type(other.body_type)
		, eyes_type(other.eyes_type)
		, mouth_type(other.mouth_type)
		, state(other.state)
		, w(other.w)
		, a(other.a)
		, s(other.s)
		, d(other.d)
		, space(other.space)
		, shift(other.shift)
		, lbtn(other.lbtn)
		, is_grounded(other.is_grounded)
		, x(other.x)
		, y(other.y)
		, z(other.z)
		, vx(other.vx)
		, vy(other.vy)
		, vz(other.vz)
		, yaw(other.yaw)
		, pitch(other.pitch)
		, roll(other.roll)
		, is_possessed(other.is_possessed)
	{ }
};

static_assert(sizeof(NetPlayerInfo) == 62, "NetObjectInfo size mismatch!");

struct NetMonsterInfo
{
	uint64			monster_id;
	uint32			room_id;

	float			x, y, z;
	float			vx = 0.0f;
	float			vy = 0.0f;
	float			vz = 0.0f;
	float			yaw = 0.0f;
	float			pitch = 0.0f;
	float			roll = 0.0f;

	AI_STATE	    AI_state;
	MON_TYPE		monster_type;

	NetMonsterInfo() = default;
	NetMonsterInfo(uint64 _id, uint32 roomId
		, MON_TYPE type
		, float _x, float _y, float _z)
		: monster_id(_id)
		, room_id(roomId)
		, AI_state(AI_STATE::MONSTER_IDLE)
		, monster_type(type)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{
	}

	NetMonsterInfo(const NetMonsterInfo& other)
		: monster_id(other.monster_id)
		, room_id(other.room_id)
		, AI_state(other.AI_state)
		, monster_type(other.monster_type)
		, x(other.x)
		, y(other.y)
		, z(other.z)
		, vx(other.vx)
		, vy(other.vy)
		, vz(other.vz)
		, yaw(other.yaw)
		, pitch(other.pitch)
		, roll(other.roll)
	{
	}
};

static_assert(sizeof(NetMonsterInfo) == 53, "NetMonsterInfo size mismatch!");


struct NetRoomInfo
{
	uint32	room_id;	// 방 ID
	char	room_name[ROOM_NAME_MAX]; // 100자
	uint16	current_player_count;
	bool	is_game_start;	// 게임이 이미 시작된 방인지

	NetRoomInfo() = default;

	NetRoomInfo(const NetRoomInfo& other)
		: room_id(other.room_id)
		, current_player_count(other.current_player_count)
		, is_game_start(other.is_game_start)
	{
		COPY_STRING(room_name, other.room_name);
	}

	NetRoomInfo(const RoomInfo& other)
		: room_id(other.room_id)
		, current_player_count(other.current_player_count)
		, is_game_start(other.is_game_start)
	{
		COPY_STRING(room_name, other.room_name);
	}
};

static_assert(sizeof(NetRoomInfo) == 57, "NetRoomInfo size mismatch!");

namespace NetPacket
{
	enum class EModelType : unsigned char
	{
		// Road Types
		ROAD = 0,      // 공원 구역의 일반 길
		PARK_GREEN,         // 공원 구역의 수풀/벤치 아래 바닥
		VILLAGE_ROAD,       // 마을(상점) 구역의 일반 길

		WALL,

		//  Buildings
		WAREHOUSE, STORE,
		DOOR,
		CORNER_DOOR,    // HOUSE_WALL_CORNER과 방향 같음

		// Building Wall Types (WareHouse, Store 공용)
		HOUSE_INNTER,    // 건물 내부 바닥
		HOUSE_WALL_STRAIGHT, // 일자 벽
		HOUSE_WALL_CORNER,   // 모서리 벽
		HOUSE_WALL_EMPTY,    // 벽X

		// Props
		KIOSK, TREE, TREASURE, BENCH, SMALL_BUSH, SEESAW, UNKNOWN
	};

	struct InstanceData
	{
		XMFLOAT3 position{};
		float rotationY{ 0.0f }; // scale 대신 회전(도 단위) 추가
		EModelType type{ EModelType::ROAD };
		EModelVariant model = EModelVariant::NONE;

		InstanceData() = default;
		InstanceData(XMFLOAT3 pos, float rot, EModelType _type, EModelVariant _model)
			: position(pos)
			, rotationY(rot)
			, type(_type)
			, model(_model)
		{ }
	};

	static_assert(sizeof(InstanceData) == 19, "InstanceData size mismatch!");
}