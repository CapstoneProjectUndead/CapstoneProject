#include "stdafx.h"
#include "QuickSlot.h"
#include "KeyManager.h"

CQuickSlot::CQuickSlot()
{
}

CQuickSlot::~CQuickSlot()
{
}

void CQuickSlot::Draw()
{
	float  scale  = G_RATIO_Y;
	ImVec2 screen = ImGui::GetIO().DisplaySize;

	float cellSz = 60.0f * scale;
	float pad    = 4.0f  * scale;
	float margin = 20.0f * scale;

	float winW = pad + (cellSz + pad) * SLOT_COUNT;
	float winH = pad + cellSz + pad;
	float winX = margin;
	float winY = screen.y - winH - margin;

	ImGui::SetNextWindowPos(ImVec2(winX, winY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar        |
		ImGuiWindowFlags_NoResize          |
		ImGuiWindowFlags_NoMove            |
		ImGuiWindowFlags_NoScrollbar       |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBackground      |
		ImGuiWindowFlags_NoFocusOnAppearing;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(0, 0));

	if (ImGui::Begin("##QuickSlot", nullptr, flags)) {
		ImGui::SetWindowFontScale(scale);
		DrawSlotCells(cellSz, pad, scale);
		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();

	ImGui::PopStyleVar(2);

	// 1~4 key: select slot
	KEY keys[SLOT_COUNT] = { KEY::_1, KEY::_2, KEY::_3, KEY::_4 };
	for (int i = 0; i < SLOT_COUNT; i++) {
		if (KEY_TAP(keys[i]) && slots[i].has_item)
			selected_slot = i;
	}
}

void CQuickSlot::DrawSlotCells(float cellSz, float pad, float scale)
{
	float       rounding = 8.0f * scale;
	ImDrawList* dl       = ImGui::GetWindowDrawList();
	ImFont*     font     = ImGui::GetFont();
	float       fontSize = ImGui::GetFontSize();

	ImVec2 origin = ImGui::GetCursorScreenPos();

	for (int i = 0; i < SLOT_COUNT; i++) {

		ImVec2 cellMin = ImVec2(origin.x + pad + i * (cellSz + pad), origin.y + pad);
		ImVec2 cellMax = ImVec2(cellMin.x + cellSz, cellMin.y + cellSz);

		// Cache position for hit testing in TryDropOnSlot
		slot_tl[i]    = cellMin;
		slot_sz_cache = cellSz;

		bool  isSelected = (selected_slot == i);
		ImU32 bgColor    = IM_COL32(50,  50,  55,  200);
		ImU32 bordColor  = isSelected ? IM_COL32(255, 220, 50,  255)
		                              : IM_COL32(130, 130, 135, 255);
		float bordWidth  = isSelected ? 2.5f * scale : 1.5f * scale;

		dl->AddRectFilled(cellMin, cellMax, bgColor,   rounding);
		dl->AddRect(      cellMin, cellMax, bordColor, rounding, 0, bordWidth);

		// Slot number 
		char numBuf[4];
		snprintf(numBuf, sizeof(numBuf), "%d", i + 1);
		float numSz  = fontSize * 0.65f;
		ImVec2 numPos = ImVec2(cellMin.x + 4.0f * scale, cellMin.y + 2.0f * scale);
		dl->AddText(font, numSz, numPos, IM_COL32(200, 200, 200, 180), numBuf);

		if (slots[i].has_item) {
			const char* name   = slots[i].name.c_str();
			float       itemSz = fontSize * 0.80f;
			ImVec2      tSz    = font->CalcTextSizeA(itemSz, FLT_MAX, 0.0f, name);
			ImVec2      tPos   = ImVec2(
				cellMin.x + (cellSz - tSz.x) * 0.5f,
				cellMin.y + (cellSz - tSz.y) * 0.5f
			);
			dl->AddText(font, itemSz, tPos, IM_COL32(230, 230, 230, 255), name);
		}

		// Invisible button
		char btnId[32];
		snprintf(btnId, sizeof(btnId), "##qs_%d", i);
		ImGui::SetCursorScreenPos(cellMin);
		ImGui::InvisibleButton(btnId, ImVec2(cellSz, cellSz));

		// 우클릭으로 슬롯 등록 해제
		if (slots[i].has_item &&
		    ImGui::IsItemHovered() &&
		    ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			slots[i] = SlotEntry{};
			if (selected_slot == i)
				selected_slot = -1;
		}
	}
}

bool CQuickSlot::TryDropOnSlot(CItem* item, ImVec2 mousePos)
{
	if (!item)
		return false;

	if (slot_sz_cache <= 0.0f)
		return false;

	// 보물 타입은 퀵슬롯 영역 위에 드롭 시 등록 없이 true 반환 (버리기 방지)
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {
		for (int i = 0; i < SLOT_COUNT; i++) {
			ImVec2 br = ImVec2(slot_tl[i].x + slot_sz_cache, slot_tl[i].y + slot_sz_cache);
			if (mousePos.x >= slot_tl[i].x && mousePos.x <= br.x &&
			    mousePos.y >= slot_tl[i].y && mousePos.y <= br.y)
				return true;
		}
		return false;
	}

	for (int i = 0; i < SLOT_COUNT; i++) {
		ImVec2 br = ImVec2(slot_tl[i].x + slot_sz_cache, slot_tl[i].y + slot_sz_cache);

		if (mousePos.x >= slot_tl[i].x && mousePos.x <= br.x &&
		    mousePos.y >= slot_tl[i].y && mousePos.y <= br.y)
		{
			// 같은 아이템이 다른 슬롯에 이미 등록되어 있으면 먼저 제거
			uint32 invId = item->GetInventoryID();
			for (int j = 0; j < SLOT_COUNT; j++) {
				if (j != i && slots[j].has_item && slots[j].inv_id == invId)
					slots[j] = SlotEntry{};
			}

			slots[i].has_item = true;
			slots[i].inv_id   = invId;
			slots[i].name     = item->GetName();
			slots[i].type     = item->GetItemType();
			return true;
		}
	}

	return false;
}

void CQuickSlot::OnItemRemovedFromInventory(uint32 inventoryId)
{
	for (int i = 0; i < SLOT_COUNT; i++) {
		if (slots[i].has_item && slots[i].inv_id == inventoryId) {
			slots[i] = SlotEntry{};
			if (selected_slot == i)
				selected_slot = -1;
		}
	}
}
