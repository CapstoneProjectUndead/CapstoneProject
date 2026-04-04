#pragma once
//==================
// ¼­¹öÂÊ ItemFactory
//==================

class CItem;

namespace ItemFactory
{
	void LoadFromJson(const std::string& path);
	std::shared_ptr<CItem> Create(int itemID);
}
