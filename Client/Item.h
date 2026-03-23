#pragma once
//==============================
// Inventory에 사용될 Item 클래스
//==============================

class CItem
{
public:
	CItem(const ItemData* data);
	virtual ~CItem();

	// UI에서 쉽게 가져다 쓸 수 있도록 Getter 제공
	const std::string& GetIconPath() const { return base_data->icon_path; }
	const std::string& GetName() const { return base_data->item_name; }
	const std::string& GetDescription() const { return base_data->description; }

protected:
	const ItemData* base_data;
};


// 장비
class CEquipment : public CItem 
{
public:
	CEquipment(const ItemData* data, const uint32 maxDur);
	virtual ~CEquipment() override;

private:
	const uint32 max_durability;
	uint32 current_durability; // 내구도
};

// 회복(음식)
class CHealItem : public CItem
{
public:
	CHealItem(const ItemData* data, const uint32 healAmount);
	virtual ~CHealItem() override;

private:
	const uint32 heal_amount;
};

// 기타(예능 아이템)
class COtherItem : public CItem
{
public:
	COtherItem(const ItemData* data);
	virtual ~COtherItem() override;

private:
	
};

// 보물
class CTreasure : public CItem
{
public:
	CTreasure::CTreasure(const ItemData* data, TREASURE_GRADE _grade);
	virtual ~CTreasure() override;

private:
	const TREASURE_GRADE grade;
};