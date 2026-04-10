#pragma once
#include "Item.h"
#include "ImGuiManager.h"

struct SlotEntry 
{
    bool        has_item = false;
    uint32      inv_id = 0;
    std::string name;
    ITEM_TYPE   type = ITEM_TYPE::ETC;
    int item_id = -1;       // 아이템 종류 ID(Rendering 시 인자로 넘겨주기 위해 필요)
};


class CQuickSlot
{
public:
    CQuickSlot();
    ~CQuickSlot();

    void Draw();

public:
    // 인벤토리에 있는 아이템을 드래그하여 퀵슬롯에 추가하는 함수
    bool TryDropOnSlot(CItem* item, ImVec2 mousePos);
    void OnItemRemovedFromInventory(uint32 inventoryId);

    int  GetSelectedSlot() const { return selected_slot; } // 0-based, -1 = none
    int GetSelectedItemId() const;
private:
    void DrawSlotCells(float cellSz, float pad, float scale);

private:
    static const int    SLOT_COUNT = 4;

    SlotEntry           slots[SLOT_COUNT];
    int                 selected_slot  = -1;

    ImVec2              slot_tl[SLOT_COUNT];
    float               slot_sz_cache  = 0.0f;
};
