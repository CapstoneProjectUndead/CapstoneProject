#pragma once
//==============================
// Inventory에 사용될 Item 클래스
//==============================

class CItem
{
public:
	CItem(std::shared_ptr<ItemData> data);
	virtual ~CItem() = 0;

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


// 장비
class CEquipment : public CItem 
{
public:
	CEquipment(const std::shared_ptr<ItemData> data, const uint32 maxDur);
	virtual ~CEquipment() override;

private:
	const uint32 max_durability;
	uint32 current_durability; // 내구도
};

// 소비(회복템)
class CCunsumable : public CItem
{
public:
	CCunsumable(const std::shared_ptr<ItemData> data, const uint32 healAmount);
	virtual ~CCunsumable() override;

private:
	const uint32 heal_amount;
};

// 기타(예능 아이템)
class COtherItem : public CItem
{
public:
	COtherItem(const std::shared_ptr<ItemData> data);
	virtual ~COtherItem() override;

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