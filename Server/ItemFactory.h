#pragma once
//==================
// 서버쪽 ItemFactory
//==================

class CItem;

namespace ItemFactory
{
	void LoadFromJson(const std::string& path);
	std::shared_ptr<CItem> Create(int itemID);
}
