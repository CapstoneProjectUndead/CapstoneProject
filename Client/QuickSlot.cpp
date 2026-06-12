#include "stdafx.h"
#include "QuickSlot.h"
#include "KeyManager.h"
#include "MyPlayer.h"
#include "ServerPacketHandler.h"

CQuickSlot::CQuickSlot()
{
}

CQuickSlot::~CQuickSlot()
{
}

int CQuickSlot::GetSelectedItemId() const
{
	if (selected_slot < 0 || selected_slot >= SLOT_COUNT)
		return -1;

	if (!slots[selected_slot].has_item)
		return -1;

	return slots[selected_slot].item_id;
}

ITEM_TYPE CQuickSlot::GetSelectedItemType() const
{
	if (selected_slot < 0 || selected_slot >= SLOT_COUNT)
		return ITEM_TYPE::NONE;

	return slots[selected_slot].type;
}

ITEM_SUB_TYPE CQuickSlot::GetSelectedSubType() const
{
	if (selected_slot < 0 || selected_slot >= SLOT_COUNT)
		return ITEM_SUB_TYPE::NONE;

	if (!slots[selected_slot].has_item)
		return ITEM_SUB_TYPE::NONE;

	return slots[selected_slot].sub_type;
}

int CQuickSlot::GetSelectedInvId() const
{
	if (selected_slot < 0 || selected_slot >= SLOT_COUNT)
		return -1;

	if (!slots[selected_slot].has_item)
		return -1;

	return static_cast<int>(slots[selected_slot].inv_id);
}

void CQuickSlot::Reset()
{
	for (int i = 0; i < SLOT_COUNT; ++i) {
		slots[i] = SlotEntry{};
	}

	selected_slot = -1;
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

	// 1~4 key: select slot (같은 키 재입력 시 해제)
	KEY keys[SLOT_COUNT] = { KEY::_1, KEY::_2, KEY::_3, KEY::_4 };

	for (int i = 0; i < SLOT_COUNT; i++) {
		if (KEY_TAP(keys[i]) && slots[i].has_item) {

			// 빈사/사망 시 장착/해제 차단
			if (auto p = owner.lock(); p && p->IsIncapacitated())
				continue;

			if (selected_slot == i) {

				// 같은 슬롯 재입력 → 장착 해제
				selected_slot = -1;
				auto player = owner.lock();
				if (!g_is_single) {
					if (player) {
						C_EquipItem equipItemPkt;
						equipItemPkt.item_id      = 0;
						equipItemPkt.inventory_id = 0;
						equipItemPkt.player_id    = player->GetID();
						equipItemPkt.scene_type   = player->GetCurrentSceneType();

						auto sendBuffer = MAKE_SEND_BUFFER(equipItemPkt);
						player->GetSession()->DoSend(sendBuffer);
					}
				}
				else {
					player->SetEquippedItemId(0);
				}
			}
			else {
				// 새 슬롯 선택
				selected_slot = i;

				auto player = owner.lock();
				if (!g_is_single) {
					if (player) {
						C_EquipItem equipItemPkt;
						equipItemPkt.item_id      = slots[i].item_id;
						equipItemPkt.inventory_id = slots[i].inv_id;
						equipItemPkt.player_id    = player->GetID();
						equipItemPkt.scene_type   = player->GetCurrentSceneType();

						auto sendBuffer = MAKE_SEND_BUFFER(equipItemPkt);
						player->GetSession()->DoSend(sendBuffer);
					}
				}
				else {
					player->SetEquippedItemId(slots[i].item_id);
				}
			}
		}
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
			const std::string& iconKey = slots[i].icon_path;
			ImTextureID tex = iconKey.empty() ? 0 : CImGuiManager::GetInstance().GetTexture(iconKey);

			if (tex) {
				float   imgPad = 6.0f * scale;
				ImVec2  imgMin = ImVec2(cellMin.x + imgPad, cellMin.y + imgPad);
				ImVec2  imgMax = ImVec2(cellMax.x - imgPad, cellMax.y - imgPad);
				dl->AddImage(tex, imgMin, imgMax);
			}
			else {
				const char* name   = slots[i].name.c_str();
				float       itemSz = fontSize * 0.80f;
				ImVec2      tSz    = font->CalcTextSizeA(itemSz, FLT_MAX, 0.0f, name);
				ImVec2      tPos   = ImVec2(
					cellMin.x + (cellSz - tSz.x) * 0.5f,
					cellMin.y + (cellSz - tSz.y) * 0.5f
				);
				dl->AddText(font, itemSz, tPos, IM_COL32(230, 230, 230, 255), name);
			}
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
			bool wasSelected = (selected_slot == i);
			slots[i] = SlotEntry{};
			if (wasSelected)
				selected_slot = -1;

			if (!g_is_single && wasSelected) {
				if (auto player = owner.lock()) {
					C_EquipItem equipItemPkt;
					equipItemPkt.item_id      = 0;
					equipItemPkt.inventory_id = 0;
					equipItemPkt.player_id    = player->GetID();
					equipItemPkt.scene_type   = player->GetCurrentSceneType();

					auto sendBuffer = MAKE_SEND_BUFFER(equipItemPkt);
					player->GetSession()->DoSend(sendBuffer);
				}
			}
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

			slots[i].has_item  = true;
			slots[i].inv_id    = invId;
			slots[i].name      = item->GetName();
			slots[i].icon_path = item->GetIconPath();
			slots[i].type      = item->GetItemType();
			slots[i].sub_type  = item->GetSubType();
			slots[i].item_id   = item->GetItemId();

			// 현재 장착 중인 슬롯에 드롭한 경우 equipped_item_id 갱신
			if (i == selected_slot) {
				auto player = owner.lock();
				if (!g_is_single) {
					if (player) {
						C_EquipItem pkt;
						pkt.item_id      = slots[i].item_id;
						pkt.inventory_id = slots[i].inv_id;
						pkt.player_id    = player->GetID();
						pkt.scene_type   = player->GetCurrentSceneType();
						auto sendBuffer = MAKE_SEND_BUFFER(pkt);
						player->GetSession()->DoSend(sendBuffer);
					}
				}
				else {
					if (player)
						player->SetEquippedItemId(slots[i].item_id);
				}
			}

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
			if (selected_slot == i) {
				selected_slot = -1;

				if (!g_is_single) {
					if (auto player = owner.lock())
						player->SetEquippedItemId(0);
				}
			}
		}
	}
}
