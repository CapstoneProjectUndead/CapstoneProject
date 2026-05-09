#include "stdafx.h"
#include "Item.h"
#include "MyPlayer.h"

CItem::CItem(std::shared_ptr<ItemData> data)
	: base_data(data)
	, inventory_id(-1)
{
}

CItem::~CItem()
{

}

// 장비 공통
CEquipment::CEquipment(const std::shared_ptr<ItemData> data, const uint32 maxDur)
	: CItem(data)
	, max_durability(maxDur)
	, current_durability(maxDur)
{
}

CEquipment::~CEquipment()
{
}

// 파밍 도구
CTool::CTool(const std::shared_ptr<ItemData> data, const uint32 maxDur)
	: CEquipment(data, maxDur)
{
}

CTool::~CTool()
{
}

void CTool::Equip(CMyPlayer* player)
{
}

void CTool::ReduceDurability()
{
	if (current_durability > 0)
		current_durability -= 5;
}

// 무기
CWeapon::CWeapon(const std::shared_ptr<ItemData> data, const uint32 maxDur)
	: CEquipment(data, maxDur)
{
}

CWeapon::~CWeapon()
{
}

void CWeapon::Equip(CMyPlayer* player)
{
}

void CWeapon::ReduceDurability()
{
}

// 소비(회복템)
CConsumable::CConsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount, const uint32 energyAmount, const uint32 effectAmount, const float buffDuration)
	: CItem(data)
	, heal_amount(healAmount)
	, energy_amount(energyAmount)
	, effect_amount(effectAmount)
	, buff_duration(buffDuration)
{
}

CConsumable::~CConsumable()
{
}

bool CConsumable::Use(CMyPlayer* player)
{
	if (!player) 
		return false;

	// 이 소비템이 hp를 올려주는 효과가 있다면
	if (heal_amount > 0)
		player->SetHp(min(player->GetHp() + heal_amount, player->GetMaxHp()));

	// 이 소비템이 기력을 올려주는 효과가 있다면
	if (energy_amount > 0)
		player->AddStamina(energy_amount);

	// 이 소비템이 버프 효과가 있다면
	if (effect_amount > 0) {
		Buff buff;
		buff.duration        = buff_duration;
		buff.miningSpeedMult = 1.f + effect_amount / 100.f;
		player->AddBuff(buff);
	}

	return true;
}

// 기타(예능 아이템)
COther::COther(const std::shared_ptr<ItemData> data)
	: CItem(data)
{
}

COther::~COther()
{
}

bool COther::Use(CMyPlayer* player)
{
	switch (GetSubType())
	{
	case ITEM_SUB_TYPE::NONE_EFFECT:
		return false;
		break;
	case ITEM_SUB_TYPE::AGGRO:
		break;
	case ITEM_SUB_TYPE::UTIL:
		break;
	case ITEM_SUB_TYPE::TRAP:
		break;
	default:
		break;
	}

	return false;
}

// 보물
CTreasure::CTreasure(const std::shared_ptr<ItemData> data, TREASURE_GRADE _grade, uint32 _price)
	: CItem(data)
	, price(_price)
	, grade(_grade)
{
}

CTreasure::~CTreasure()
{
}
