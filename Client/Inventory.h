#pragma once
#include "Item.h"

class CMyPlayer;

class CInventory
{
public:
	CInventory(std::shared_ptr<CMyPlayer> owner);
	CInventory(const CInventory&) = delete;
	~CInventory();

public:
	void  AddItem(std::shared_ptr<CItem> item);
	void  RemoveItem(int itemID);

	void  ToggleOpen() { is_open = !is_open; }
	bool  IsOpen() const { return is_open; }

	float GetCurrentWeight() const { return current_weight; }
	float GetMaxWeight() const { return max_weight; }
	void  UpgradeMaxWeight(float amount) { max_weight += amount; }

	void  Draw();

	void  SetDropCallback(std::function<void(std::shared_ptr<CItem>)> cb) { on_drop_callback = cb; }

private:
	void BeginDrawInventory();
	void DrawTitleBar(float winW, float titleH); // 타이틀 + X 버튼
	void DrawTabBar();                           // 탭 4개 + 아이템 테이블
	void DrawItemTable(ITEM_TYPE type);          // 보물 탭 - 이름/무게 테이블
	void DrawItemGrid(ITEM_TYPE type);           // 장비/회복/기타 탭 - 이미지 격자
	void DrawBottomBar();                        // 하단 용량 + 소지금 바

private:
	std::weak_ptr<CMyPlayer>             owner;          // 소지금 접근용

	std::vector<std::shared_ptr<CItem>>  items;
	float                                current_weight = 0.0f;
	float                                max_weight     = 200.0f; // 기본값, 업그레이드로 증가

	bool                                 is_open        = false;
	ITEM_TYPE                            active_tab     = ITEM_TYPE::EQUIPMENT;

	// 드래그 상태
	CItem*                               dragged_item   = nullptr;
	bool                                 is_dragging    = false;

	std::function<void(std::shared_ptr<CItem>)> on_drop_callback;
};
