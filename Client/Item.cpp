#include "stdafx.h"
#include "Item.h"

CItem::CItem(const ItemData* data)
	: base_data(data)
{
}

CItem::~CItem()
{
	if (base_data != nullptr) {
		delete base_data;
		base_data = nullptr;
	}
}

// 장비
CEquipment::CEquipment(const ItemData* data, uint32 maxDur)
	: CItem(data) 
	, max_durability(maxDur)
{
	current_durability = max_durability; // 생성 시 최대 내구도로 채움
}

CEquipment::~CEquipment()
{
}

// 회복(음식)
CHealItem::CHealItem(const ItemData* data, const uint32 healAmount)
	: CItem(data)
	, heal_amount(healAmount)
{
}

CHealItem::~CHealItem()
{
}

// 기타(예능 아이템)
COtherItem::COtherItem(const ItemData* data)
	: CItem(data)
{
}

COtherItem::~COtherItem()
{
}

// 보물
CTreasure::CTreasure(const ItemData* data, TREASURE_GRADE _grade)
	: CItem(data)
	, grade(_grade)
{
}

CTreasure::~CTreasure()
{
}
