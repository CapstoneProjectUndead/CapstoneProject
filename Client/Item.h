#pragma once
//==============================
// Inventory에 사용될 Item 클래스
//==============================

class CMyPlayer;

class CItem
{
public:
	CItem(std::shared_ptr<ItemData> data);
	virtual ~CItem() = 0;

	virtual bool Use(CMyPlayer* player) { return false; }

public:
	// UI에서 쉽게 가져다 쓸 수 있도록 Getter 제공
	int                GetItemId() const { return base_data->item_id; }
	ITEM_TYPE          GetItemType() const { return base_data->item_type; }
	ITEM_SUB_TYPE	   GetSubType() const { return base_data->item_sub_type; }
	float              GetWeight() const { return base_data->weight; }
	const std::string& GetIconPath() const { return base_data->icon_path; }
	const std::string& GetName() const { return base_data->item_name; }
	const std::string& GetDescription() const { return base_data->description; }

	uint32	GetInventoryID() const { return inventory_id; }
	void	SetInventoryID(const uint32 id) { inventory_id = id; }

protected:
	const std::shared_ptr<ItemData> base_data;
	uint32							inventory_id;
};


// 장비 공통
class CEquipment : public CItem
{
public:
	CEquipment(const std::shared_ptr<ItemData> data, const uint32 maxDur);
	virtual ~CEquipment() = 0;

	virtual void Equip(CMyPlayer* player) = 0;

public:
	uint32 GetMaxDurability() const { return max_durability; }
	uint32 GetCurrentDurability() const { return current_durability; }
	void   SetCurrentDurability(const uint16 dur) { current_durability = dur; }

	virtual void ReduceDurability() {};

	// 멀티용
	void SetCurrentDurability(uint32 dur) { current_durability = dur; }

protected:
	const uint32 max_durability;
	uint32       current_durability;
};

// 파밍 도구 (내구도 있음)
class CTool : public CEquipment
{
public:
	CTool(const std::shared_ptr<ItemData> data, const uint32 maxDur);
	virtual ~CTool() override;

	virtual void Equip(CMyPlayer* player) override;
	virtual void ReduceDurability() override;
};

// 무기
class CWeapon : public CEquipment
{
public:
	CWeapon(const std::shared_ptr<ItemData> data, const uint32 maxDur, const uint32 atk);
	virtual ~CWeapon() override;

	virtual void Equip(CMyPlayer* player) override;
	virtual void ReduceDurability() override;

	uint32 GetAttackPower() const { return attack_power; }

private:
	const uint32 attack_power;
};

// 소비(회복템)
class CConsumable : public CItem
{
public:
	CConsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount, const uint32 energyAmount, const uint32 effectAmount, const float buffDuration);
	virtual ~CConsumable() override;

	virtual bool Use(CMyPlayer* player);

public:
	uint32 GetHealAmount()   const { return heal_amount; }
	uint32 GetEnergyAmount() const { return energy_amount; }
	uint32 GetEffectAmount() const { return effect_amount; }

private:
	const uint32 heal_amount;
	const uint32 energy_amount;
	const uint32 effect_amount;
	const float  buff_duration;
};

// 기타(예능 아이템)
class COther : public CItem
{
public:
	COther(const std::shared_ptr<ItemData> data);
	virtual ~COther() override;

	virtual bool Use(CMyPlayer* player);

private:
	
};

// 보물
class CTreasure : public CItem
{
public:
	CTreasure::CTreasure(const std::shared_ptr<ItemData> data, TREASURE_GRADE _grade, uint32 _price);
	virtual ~CTreasure() override;

	TREASURE_GRADE GetGrade() const { return grade; }
	uint32         GetPrice() const { return price; }

private:
	const uint32		 price;
	const TREASURE_GRADE grade;
};