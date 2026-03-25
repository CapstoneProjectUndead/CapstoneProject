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
	UI,	// ui 생성용

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

// Map 관련
enum class EModelVariant : uint16_t
{
	NONE = 0,

	// 고정 맵 에셋 (Fixed)
	PARK_ROAD, PARK_GREEN, VILLAGE_ROAD, HOUSE_PLACE,
	WALL_1002, WALL_2001, WALL_1003,
	WALL_1_DOOR001, WALL_2_DOOR001,
	VENDING_MACHINE_001, SEESAW_001,

	// Grass
	GRASS_019, GRASS_020, GRASS_021, GRASS_022, GRASS_023, GRASS_024,
	GRASS_025, GRASS_026, GRASS_027, GRASS_028, GRASS_029, GRASS_030,
	GRASS_031, GRASS_032, GRASS_033, GRASS_034, GRASS_035, GRASS_036, GRASS_037,

	// Stone
	STONE_011, STONE_012, STONE_013, STONE_014, STONE_015, STONE_016,
	STONE_017, STONE_018, STONE_019, STONE_020, STONE_021, STONE_022,
	STONE_023, STONE_024,

	// Props
	PARK_BENCH_002, PARK_BENCH_003,
	SMALL_BUSH_001, SMALL_BUSH_002,
	TREE_002, PINETREE,
	TRASHCAN_001, TRASHCAN_002,

	COUNT
};

// 아이템 관련

enum class ITEM_TYPE : uint8_t
{

};