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
	MINEABLE_OBJECT,
};

enum class PLAYER_STATE : uint8_t
{
	IDLE,
	WALK,
	RUN,
	JUMP,
	DIG,
	ATTACK,
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

enum class AI_STATE // 어떤 애니메이션을 재생할지 결정한다.
{
	MONSTER_IDLE,
	MONSTER_PATROL,
	MONSTER_TRACE,
	MONSTER_ATTACK,
	MONSTER_FLEE,
	MONSTER_DEAD,
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
	STONE_WALL, PARK_WALL,
	TENT_CLOTH,
	TENT_CORNER_CLOTH,
	TENT_CORNER,

	// Grass
	GRASS_1, GRASS_2, GRASS_3,

	// Stone
	STONE_011, STONE_012, STONE_013, STONE_014, STONE_015, STONE_016,
	STONE_017, STONE_018, STONE_019, STONE_020, STONE_021, STONE_022,
	STONE_023, STONE_024,

	// Props
	PARK_BENCH_002, PARK_BENCH_003,
	PARK_BUSH, PARK_SHRUB,
	TREE_1, TREE_2, TREE_3,
	TRASHCAN_001, TRASHCAN_002,
	MANHOLE,

	BOOKSHELF,
	FENCE_WOOD,
	REFRIGERATOR,
	REFRIGERATOR_DOOR_LOWER,
	REFRIGERATOR_DOOR_UPPER,
	SOFA,
	STREETLAMP,
	TABLE_LOW,
	CHAIR_001,
	TABLE_001,

	PARK_SAND,

	CRATE_1,
	CRATE_2,
	DRUM,

	COUNT
};


// 아이템 관련

// 대분류: 클래스 분기용 (CEquipment로 만들지, CHealItem으로 만들지 결정)
enum class ITEM_TYPE : uint8_t
{
	EQUIPMENT  = 0,
	CONSUMABLE,
	ETC,
	TREASURE,
	NONE,       // 슬롯 비어있음/유효하지 않은 타입을 나타내는 센티널
};

// 소분류: 실제 동작 분기용 (도끼인지, 드릴인지 결정)
enum class ITEM_SUB_TYPE : uint16_t
{
	NONE = 0,

	// (Equipment 시리즈)-파밍 도구 시리즈(100번대)
	TOOL = 100,

	// (Equipment 시리즈)-무기 시리즈 (150번대)
	MELEE_WEAPON  = 150,
	RANGED_WEAPON = 151,

	// 소비 아이템 시리즈 (200번대)
	HEAL = 200,
	ENERGY,
	HEAL_ENERGY,
	BUFF,

	// 예능 아이템 시리즈 (300번대)
	NONE_EFFECT = 300,
	AGGRO,
	UTIL,
	TRAP,
};

//=========================================================================
// DB 테이블에 item_type 컬럼과 sub_type 컬럼을 숫자로 저장해두면,
// 서버가 켜질 때 이 값을 읽어서 바로 Enum으로 캐스팅(static_cast)할 수 있습니다.

//  예: 1(EQUIPMENT), 100(AXE) -> "아, 이 데이터는 장비이고 파밍 도구이구나."
//=========================================================================

enum class TREASURE_GRADE : uint8_t
{
	COMMON = 0,	// 일반
	UNCOMMON,	// 고급
	RARE,		// 레어
	EPIC,		// 에픽
	LEGENDARY	// 전설
};

enum class MINEABLEOBJECT_TYPE
{
	NONE,
	VISIBLE,
	NONE_VISIBLE
};

//=======
// 사운드
//=======

enum class SOUND_ID : uint16_t
{
	button01a,
	damaged1,
	ghost_attack,
	jump12,
	select09,
	crude_laughter,
	devil_laugh1,
	flying_pan,
	jab,
	ghost_spray,
	devil_scared1,
	surprising_girl,
	ridicule,
	swing2,
	sword,
	dog_bark,
	dog_attack,
	dog_moan,
	dog_howling,
	girl_flee,
	clock_alarm,
	pick_up,
	warning_bell,
};