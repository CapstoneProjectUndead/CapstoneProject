#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

struct InputData 
{
	// 이동 관련 (의도)
	bool w = false;
	bool a = false;
	bool s = false;
	bool d = false;

	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;
};

struct ObjectInfo
{
	// 서버가 처리 완료한 이 플레이어의 마지막 시퀀스 번호
	uint64			last_seq_num;

	uint32			id;
	float			x, y, z;
	float			yaw, pitch, roll;
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
		: id(other.id)
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