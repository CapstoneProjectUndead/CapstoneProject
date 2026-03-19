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
	Vaild, // 파밍 가능한 보물
	Invalid, // 파밍 불가능 
	Occupied, // 누가 지금 파고있다.
};


// 아이템 관련

enum class ITEM_TYPE : uint8_t
{

};