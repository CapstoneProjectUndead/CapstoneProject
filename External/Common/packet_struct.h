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

struct NetObjectInfo
{
	uint32			id;

	// 서버권위 방식) w,a,s,d 서버권위 방식에서 필요한 데이터이다.
	bool			w = false;
	bool			a = false;
	bool			s = false;
	bool			d = false;

	float			x, y, z;
	float			vx, vy, vz;
	float			pitch	= 0.0f;
	float			yaw		= 0.0f;
	float			roll	= 0.0f; 

	PLAYER_STATE	state;

	NetObjectInfo() = default;
	NetObjectInfo(int _id, float _x, float _y, float _z)
		: id(_id)
		, state(PLAYER_STATE::IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{ }

	NetObjectInfo(const NetObjectInfo& other)
		: id(other.id)
		, state(other.state)
		, w(other.w)
		, a(other.a)
		, s(other.s)
		, d(other.d)
		, x(other.x)
		, y(other.y)
		, z(other.z)
		, vx(other.vx)
		, vy(other.vy)
		, vz(other.vz)
		, yaw(other.yaw)
		, pitch(other.pitch)
		, roll(other.roll)
	{ }
};

static_assert(sizeof(NetObjectInfo) == 45, "PackObjectInfo size mismatch!");