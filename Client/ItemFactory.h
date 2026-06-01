#pragma once

class CItem;
struct ItemData;

namespace ItemFactory
{
	std::string GetModelName(int itemId);
	void LoadModelMap(const std::string& path);
	void LoadFromJson(const std::string& path);
	std::shared_ptr<CItem> Create(int itemID);

	// id로 아이템 메타데이터(이름/아이콘/가격 등) 조회. 없으면 nullptr.
	const ItemData* GetItemData(int itemID);
}
