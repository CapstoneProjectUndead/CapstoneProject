#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

#define ROOM_NAME_MAX 50

struct InputData 
{
	// 이동 관련 (서버가 검증)
	bool w = false;
	bool a = false;
	bool s = false;
	bool d = false;
	bool space = false;
};

struct PlayerStat
{
	uint32 hp;
	uint32 stamina;
	uint16 miningSpeed;

	const uint32 maxHp = 1000;
	const uint32 maxStamina = 1000;
};

struct Buff
{
	float duration;        // 남은 시간 (초)
	float miningSpeedMult; // 채굴 속도 배율 (50% 증가)
};

struct PlayerInfo
{
	// 서버가 처리 완료한 이 플레이어의 마지막 시퀀스 번호
	uint64			last_seq_num;
	uint32			id;

	// 서버권위 방식) InputData는 서버권위 방식에서 필요한 데이터이다.
	InputData		input;
	bool            is_grounded = true;

	float			x, y, z;	// 좌표
	float			vx, vy, vz; // velocity

	float			pitch = 0.0f;
	float			yaw = 0.0f;
	float			roll = 0.0f;

	PLAYER_STATE	state;

	PlayerInfo() = default;
	PlayerInfo(int _id, float _x, float _y, float _z)
		: id(_id)
		, state(PLAYER_STATE::IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{
	}

	PlayerInfo(uint64 seqNum, int _id, float _x, float _y, float _z)
		: last_seq_num(seqNum)
		, id(_id)
		, state(PLAYER_STATE::IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{
	}

	PlayerInfo(const PlayerInfo& other)
		: last_seq_num(other.last_seq_num)
		, id(other.id)
		, input(other.input)
		, state(other.state)
		, x(other.x)
		, y(other.y)
		, z(other.z)
		, yaw(other.yaw)
		, pitch(other.pitch)
		, roll(other.roll)
	{
	}
};

struct MonsterInfo
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

	MonsterInfo() = default;
	MonsterInfo(uint64 _id, uint32 roomId
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
};

struct RoomInfo
{
	uint32	room_id;	// 방 ID
	char	room_name[ROOM_NAME_MAX]; // 50자
	uint16	current_player_count;
	bool	is_game_start;	// 게임이 이미 시작된 방인지

	RoomInfo() = default;
	RoomInfo(uint32 id, const char* name, uint16 cnt, bool gameStart)
		: room_id(id)
		, current_player_count(cnt)
		, is_game_start(gameStart)
	{
		strncpy_s(room_name, name, ROOM_NAME_MAX - 1);
	}
};

struct TreasureInfo
{
	uint32         world_id;
	XMFLOAT3       treasure_pos;
	TREASURE_STATE treasure_state;

	TreasureInfo() = default;

	TreasureInfo(const XMFLOAT3 pos)
		: world_id(0)
		, treasure_pos(pos)
		, treasure_state(TREASURE_STATE::Vaild)
	{}

	TreasureInfo(uint32 id, const XMFLOAT3 pos)
		: world_id(id)
		, treasure_pos(pos)
		, treasure_state(TREASURE_STATE::Vaild)
	{}
};


// 아이템 

struct ItemData 
{
	uint16		    item_id;
	ITEM_TYPE		item_type;
	ITEM_SUB_TYPE	item_sub_type;
	std::string		item_name;

	std::string		icon_path;		// 인벤토리 UI에서 그릴 이미지 경로
	std::string		mesh_path;		// 땅에 떨어졌을 때 그릴 3D 모델 경로
	std::string		description;	// 아이템 설명 (툴팁용)

	uint16			max_stack;		// 99개씩 겹칠 수 있는지 등
	uint32			price;			// 상점 판매가
	float           weight;			// 무게
	bool            is_throwable;	// 던질 수 있는지.
};

// animation 임계값
namespace AnimationThres
{
	static constexpr float IdleToWalk = 0.35f;
	static constexpr float WalkToIdle = 0.20f;

	static constexpr float WalkToRun = 0.80f;
	static constexpr float RunToWalk = 0.65f;
}