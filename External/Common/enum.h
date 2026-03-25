#pragma once
//======================================
// **** 클라/서버 공동 참조 헤더 파일 ****
//======================================

enum class OBJECT_TYPE : uint8_t
{
	STATIC_OBJECT,
	PLAYER,
	MONSTER,
	WORLD_ITEM,
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

// 대분류: 클래스 분기용 (CEquipment로 만들지, CHealItem으로 만들지 결정)
enum class ITEM_TYPE : uint8_t
{
	EQUIPMENT = 0,
	CONSUMABLE,
	TREASURE,
	ETC
};

// 소분류: 실제 동작 분기용 (도끼인지, 드릴인지 결정)
enum class ITEM_SUB_TYPE : uint16_t
{
	NONE = 0,

	// Equipment 시리즈 (100번대)
	EQUIP_AXE = 100,
	EQUIP_PICKAXE,
	EQUIP_DRILL,

	// 회복 아이템 시리즈 (200번대)
	HEAL_POTION = 200,
	HEAL_BREAD,

	// 예능 아이템 시리즈 (300번대)
	BONE = 300,

	// Treasure 시리즈 (400번대)
	TREASURE_SILVER = 400,
	TREASURE_GOLD
};

//=========================================================================
// DB 테이블에 item_type 컬럼과 sub_type 컬럼을 숫자로 저장해두면,
// 서버가 켜질 때 이 값을 읽어서 바로 Enum으로 캐스팅(static_cast)할 수 있습니다.

//  예: 1(EQUIPMENT), 101(EQUIP_DRILL) -> "아, 이 데이터는 드릴 장비구나!"
//=========================================================================

enum class TREASURE_GRADE : uint8_t
{
	COMMON = 0,	// 일반
	UNCOMMON,	// 고급
	RARE,		// 레어
	EPIC,		// 에픽
	LEGENDARY	// 전설
};