#pragma once
//======================================
// **** 클라/서버 공동 참조 헤더 파일 ****
//======================================

enum class OBJECT_TYPE : uint8_t
{
	STATIC_OBJECT,
	PLAYER,
	MONSTER,
};

enum class PLAYER_STATE : uint8_t
{
	IDLE,
	WALK,
	RUN,
	DEAD,
};

enum class SCENE_TYPE : uint8_t
{
	NONE,
	TITLE,
	CUSTOMS,
	LOBBY,
	GAME,

	END
};

enum class MON_TYPE : uint8_t
{
	HUMAN_MONSTER,
	ANIMAL_MONSTER,
	GHOST
};

enum class AI_STATE
{
	MONSTER_IDLE,
	MONSTER_PATROL,
	MONSTER_TRACE,
	MONSTER_ATTACK,
	MONSTER_DEAD,
};

enum class TREASURE_STATE : uint8_t
{
	Vaild,
	Invalid,
	Occupied,
};