#include "pch.h"
//============
// 서버쪽 Item
//============
#include "Item.h"

CItem::CItem(std::shared_ptr<ItemData> data)
	: base_data(data)
	, inventory_id(-1)
{
}

CItem::~CItem()
{
}

// 장비 공통
CEquipment::CEquipment(const std::shared_ptr<ItemData> data)
	: CItem(data)
{
}

CEquipment::~CEquipment()
{
}

// 파밍 도구
CTool::CTool(const std::shared_ptr<ItemData> data, const uint32 maxDur)
	: CEquipment(data)
	, max_durability(maxDur)
{
	current_durability = max_durability;
}

CTool::~CTool()
{
}

// 무기
CWeapon::CWeapon(const std::shared_ptr<ItemData> data)
	: CEquipment(data)
{
}

CWeapon::~CWeapon()
{
}

// 소비(회복템)
CConsumable::CConsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount, const uint32 energyAmount, const uint32 effectAmount)
	: CItem(data)
	, heal_amount(healAmount)
	, energy_amount(energyAmount)
	, effect_amount(effectAmount)
{
}

CConsumable::~CConsumable()
{
}

// 기타(예능 아이템)
COther::COther(const std::shared_ptr<ItemData> data)
	: CItem(data)
{
}

COther::~COther()
{
}

// 보물
CTreasure::CTreasure(const std::shared_ptr<ItemData> data, TREASURE_GRADE _grade)
	: CItem(data)
	, grade(_grade)
{
}

CTreasure::~CTreasure()
{
}
