#pragma once
//==============================
// Inventory에 사용될 Item 클래스
//==============================

class CItem
{
public:
	CItem(std::shared_ptr<ItemData> data);
	virtual ~CItem() = 0;

public:
	// UI에서 쉽게 가져다 쓸 수 있도록 Getter 제공
	int                GetItemId() const { return base_data->item_id; }
	ITEM_TYPE          GetItemType() const { return base_data->item_type; }
	float              GetWeight() const { return base_data->weight; }
	const std::string& GetIconPath() const { return base_data->icon_path; }
	const std::string& GetName() const { return base_data->item_name; }
	const std::string& GetDescription() const { return base_data->description; }

protected:
	const std::shared_ptr<ItemData> base_data;
};


// 장비 공통 
class CEquipment : public CItem
{
public:
	CEquipment(const std::shared_ptr<ItemData> data);
	virtual ~CEquipment() = 0;

	ITEM_SUB_TYPE GetSubType() const { return base_data->item_sub_type; }
};

// 파밍 도구 (내구도 있음)
class CTool : public CEquipment
{
public:
	CTool(const std::shared_ptr<ItemData> data, const uint32 maxDur);
	virtual ~CTool() override;

	uint32 GetCurrentDurability() const { return current_durability; }
	uint32 GetMaxDurability() const { return max_durability; }

private:
	const uint32 max_durability;
	uint32 current_durability;
};

// 무기
class CWeapon : public CEquipment
{
public:
	CWeapon(const std::shared_ptr<ItemData> data);
	virtual ~CWeapon() override;
};

// 소비(회복템)
class CConsumable : public CItem
{
public:
	CConsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount, const uint32 energyAmount, const uint32 effectAmount);
	virtual ~CConsumable() override;

	uint32 GetHealAmount()   const { return heal_amount; }
	uint32 GetEnergyAmount() const { return energy_amount; }
	uint32 GetEffectAmount() const { return effect_amount; }

private:
	const uint32 heal_amount;
	const uint32 energy_amount;
	const uint32 effect_amount;
};

// 기타(예능 아이템)
class COther : public CItem
{
public:
	COther(const std::shared_ptr<ItemData> data);
	virtual ~COther() override;

private:
	
};

// 보물
class CTreasure : public CItem
{
public:
	CTreasure::CTreasure(const std::shared_ptr<ItemData> data, TREASURE_GRADE _grade);
	virtual ~CTreasure() override;

private:
	const TREASURE_GRADE grade;
};