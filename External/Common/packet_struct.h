#pragma once
//==================================
// **** 클라/서버 공용 헤더 파일 ****
//==================================

// 네트워크 패킷 전용 구조체 선언하는 헤더
// 네이밍 규칙 : Pack + 구조체 이름 

// **** 패킷에 포함할 구조체만 선언! ****

struct PackObjectInfo
{
	uint32			id;

	// 서버권위 방식) InputData는 서버권위 방식에서 필요한 데이터이다.
	InputData		input;

	float			x, y, z;
	float			pitch	= 0.0f;
	float			yaw		= 0.0f;
	float			roll	= 0.0f; 

	PLAYER_STATE	state;

	PackObjectInfo() = default;
	PackObjectInfo(int _id, float _x, float _y, float _z)
		: id(_id)
		, state(PLAYER_STATE::IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{ }

	PackObjectInfo(const PackObjectInfo& other)
		: id(other.id)
		, state(other.state)
		, x(other.x)
		, y(other.y)
		, z(other.z)
		, yaw(other.yaw)
		, pitch(other.pitch)
		, roll(other.roll)
	{ }
};

static_assert(sizeof(PackObjectInfo) == 33, "PackObjectInfo size mismatch!");