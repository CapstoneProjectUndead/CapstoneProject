#pragma once

enum STATE
{
	IDLE,
	WALK,
	RUN,
};

struct ObjectInfo
{
	int	  id;
	float x, y, z;
	float yaw, pitch, roll;
	STATE state;

	ObjectInfo() = default;
	ObjectInfo(int _id, float _x, float _y, float _z)
		: id(_id)
		, state(IDLE)
		, x(_x)
		, y(_y)
		, z(_z)
		, yaw{}
		, pitch{}
		, roll{}
	{ }

	ObjectInfo(const ObjectInfo& other)
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