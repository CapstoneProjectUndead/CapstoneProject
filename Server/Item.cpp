#include "pch.h"
//============
// 서버쪽 Item
//============
#include "Item.h"
#include "Player.h"

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

bool CConsumable::Use(CPlayer* player)
{
	if (!player)
		return false;

	if (heal_amount > 0)
		player->SetHp(min(player->GetHp() + heal_amount, player->GetMaxHp()));

	if (energy_amount > 0)
		player->AddStamina(energy_amount);

	// 이 소비템이 버프 효과가 있다면
	if (effect_amount > 0) {
		Buff buff;
		buff.duration = buff_duration;
		buff.miningSpeedMult = 1.f + effect_amount / 100.f;
		//player->AddBuff(buff);
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

bool COther::Use(CPlayer* player)
{
	// 채굴속도 버프 등 추후 구현
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
