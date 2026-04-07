#pragma once

class CItem;

namespace ItemFactory
{
	std::string GetModelName(int itemId);
	void LoadModelMap(const std::string& path);
	void LoadFromJson(const std::string& path);
	std::shared_ptr<CItem> Create(int itemID);
}
