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
};

struct ObjectInfo
{
	// 서버가 처리 완료한 이 플레이어의 마지막 시퀀스 번호
	uint64			last_seq_num;
	uint32			id;

	// 서버권위 방식) InputData는 서버권위 방식에서 필요한 데이터이다.
	InputData		input;

	float			x, y, z;	// 좌표
	float			vx, vy, vz; // velocity

	float			pitch = 0.0f;
	float			yaw = 0.0f;
	float			roll = 0.0f;

	PLAYER_STATE	state;

	ObjectInfo() = default;
	ObjectInfo(int _id, float _x, float _y, float _z)
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

	ObjectInfo(uint64 seqNum, int _id, float _x, float _y, float _z)
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

	ObjectInfo(const ObjectInfo& other)
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