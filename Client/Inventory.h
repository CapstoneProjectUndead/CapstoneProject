#pragma once
#include "Item.h"

class CMyPlayer;
class CQuickSlot;

class CInventory
{
public:
	CInventory(std::shared_ptr<CMyPlayer> owner);
	CInventory(const CInventory&) = delete;
	~CInventory();

public:
	bool  AddItem(std::shared_ptr<CItem> item);
	void  AddItemWithId(std::shared_ptr<CItem> item, uint32 inventoryId);
	void  RemoveItem(uint32 inventoryId);

	const std::unordered_map<uint32, std::shared_ptr<CItem>>& GetItems() const { return items; }

	void  ToggleOpen() { is_open = !is_open; }
	void  SetOpen(bool open) { is_open = open; }
	bool  IsOpen() const { return is_open; }

	// 상점 등에서 인벤토리를 특정 위치(좌상단)에 강제 배치할 때 사용. 해제 시 기본(우하단) 복귀.
	void  SetPositionOverride(float x, float y) { has_pos_override = true; pos_override_x = x; pos_override_y = y; }
	void  ClearPositionOverride() { has_pos_override = false; }

	uint32 GetCurrentWeight() const { return current_weight; }
	uint32 GetMaxWeight() const { return max_weight; }
	void   UpgradeMaxWeight(float amount) { max_weight += static_cast<uint32>(amount); }
	void   SetMaxWeight(uint32 weight) { max_weight = weight; }   // 절대값 설정 (서버 동기화/로드용)

	void  Draw(bool viewOnly = false); // view_only=true: 드래그/드롭/퀵슬롯 등록 차단 (LobbyScene 등 조회 전용)

	void  SetDropCallback(std::function<void(std::shared_ptr<CItem>)> cb) { on_drop_callback = cb; }
	void  SetQuickSlot(std::shared_ptr<CQuickSlot> qs) { quick_slot = qs; }

private:
	void BeginDrawInventory();
	void DrawTitleBar(float winW, float titleH); // 타이틀 + X 버튼
	void DrawTabBar();                           // 탭 4개 + 아이템 테이블
	void DrawItemTable(ITEM_TYPE type);          // 보물 탭 - 이름/무게 테이블
	void DrawItemGrid(ITEM_TYPE type);           // 장비/회복/기타 탭 - 이미지 격자
	void DrawBottomBar();                        // 하단 용량 + 소지금 바
	void DrawItemTooltip(CItem* item);           // 아이템 툴팁

private:
	std::weak_ptr<CMyPlayer>							owner;          // 소지금 접근용

	std::unordered_map<uint32, std::shared_ptr<CItem>>  items;
	uint32                                              inventory_id_counter = 0;
	uint32                                              current_weight = 0;
	uint32												max_weight = 200; // 기본값, 업그레이드로 증가

	bool												is_open        = false;
	bool												view_only      = false; // true: 드래그/드롭/퀵슬롯 등록 차단 (LobbyScene 등)

	bool												has_pos_override = false; // 상점 옆 배치 등 위치 강제 여부
	float												pos_override_x = 0.f;
	float												pos_override_y = 0.f;
	ITEM_TYPE											active_tab     = ITEM_TYPE::EQUIPMENT;

	// 드래그 상태
	CItem*												dragged_item   = nullptr;
	bool												is_dragging    = false;

	std::function<void(std::shared_ptr<CItem>)> on_drop_callback;
	std::shared_ptr<CQuickSlot>                 quick_slot;
};
