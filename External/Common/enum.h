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
};

enum class SCENE_TYPE
{
	TITLE,
	CUSTOMS,
	LOBBY,
	GAME,

	END
};

enum class MON_STATE
{
	IDLE,
	PATROL,
	TRACE,
	ATTACK,
	DEAD,
};