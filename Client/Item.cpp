#include "stdafx.h"
#include "Item.h"

CItem::CItem(std::shared_ptr<ItemData> data)
	: base_data(data)
{
}

CItem::~CItem()
{

}

// 장비
CEquipment::CEquipment(const std::shared_ptr<ItemData> data, uint32 maxDur)
	: CItem(data) 
	, max_durability(maxDur)
{
	current_durability = max_durability; // 생성 시 최대 내구도로 채움
}

CEquipment::~CEquipment()
{
}

// 회복(음식)
CCunsumable::CCunsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount)
	: CItem(data)
	, heal_amount(healAmount)
{
}

CCunsumable::~CCunsumable()
{
}

// 기타(예능 아이템)
COtherItem::COtherItem(const std::shared_ptr<ItemData> data)
	: CItem(data)
{
}

COtherItem::~COtherItem()
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
