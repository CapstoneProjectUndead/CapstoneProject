#include "stdafx.h"
#include "Inventory.h"
#include "ImGuiManager.h"
#include "MyPlayer.h"
#include "ServerPacketHandler.h"
#include "QuickSlot.h"

#undef max
#undef min

CInventory::CInventory(std::shared_ptr<CMyPlayer> _owner)
	: owner(_owner)
{
}

CInventory::~CInventory()
{
}

// 싱글용
bool CInventory::AddItem(std::shared_ptr<CItem> item)
{
	// 보물만 무게에 영향을 준다.
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {

		// 이미 꽉 찼다면 return!
		if (current_weight >= max_weight)
			return false;

		// 이 보물을 추가했을 때 최대 무게를 초과하면 거부
		if (current_weight + item->GetWeight() > max_weight)
			return false;

		current_weight += item->GetWeight();
	}

	uint32 id = inventory_id_counter++;
	item->SetInventoryID(id);
	items[id] = std::move(item);

	return true;
}

// 멀티용
void CInventory::AddItemWithId(std::shared_ptr<CItem> item, uint32 inventoryId)
{
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {

		if (current_weight >= max_weight)
			return;

		if (current_weight + item->GetWeight() > max_weight)
			return;

		current_weight += item->GetWeight();
	}

	item->SetInventoryID(inventoryId);
	items[inventoryId] = std::move(item);
}

void CInventory::RemoveItem(uint32 inventoryId)
{
	auto it = items.find(inventoryId);
	if (it == items.end())
		return;

	// 보물이면 가방 무게에서 제외
	if (it->second->GetItemType() == ITEM_TYPE::TREASURE)
		current_weight -= it->second->GetWeight();

	// 퀵슬롯에 등록된 아이템이면 슬롯 비움
	if (quick_slot)
		quick_slot->OnItemRemovedFromInventory(inventoryId);

	items.erase(it);
}

void CInventory::Draw()
{
	if (!is_open)
		return;

	BeginDrawInventory();
}

void CInventory::BeginDrawInventory()
{
	float  scale = G_RATIO_Y;
	ImVec2 screen = ImGui::GetIO().DisplaySize;
	float  winW = 348.0f * scale;
	float  winH = 460.0f * scale;
	float  titleH = 36.0f * scale;

	float  margin = 20.0f * scale;
	ImGui::SetNextWindowPos(ImVec2(screen.x - winW - margin, screen.y - winH - margin), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoMove;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f * scale);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.15f, 0.15f, 0.15f, 0.85f));

	if (ImGui::Begin("##Inventory", nullptr, flags)) {

		ImGui::SetWindowFontScale(scale);

		DrawTitleBar(winW, titleH);
		DrawTabBar();
		DrawBottomBar();

		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::End();

	ImGui::PopStyleColor(); // WindowBg
	ImGui::PopStyleVar(3);  // WindowPadding, ItemSpacing, WindowRounding

	// 드래그 프리뷰: 마우스 커서 위치에 아이템 ghost 렌더링
	if (is_dragging && dragged_item) {
		ImVec2      mousePos = ImGui::GetMousePos();
		float       ghostSz  = 50.0f * scale;
		ImVec2      ghostMin = ImVec2(mousePos.x - ghostSz * 0.5f, mousePos.y - ghostSz * 0.5f);
		ImVec2      ghostMax = ImVec2(mousePos.x + ghostSz * 0.5f, mousePos.y + ghostSz * 0.5f);
		ImDrawList* dl       = ImGui::GetForegroundDrawList();

		dl->AddRectFilled(ghostMin, ghostMax, IM_COL32(210, 210, 215, 200), 6.0f * scale);
		dl->AddRect(ghostMin, ghostMax,       IM_COL32(120, 120, 125, 255), 6.0f * scale);

		const std::string& ghostIconKey = dragged_item->GetIconPath();
		ImTextureID ghostTex = ghostIconKey.empty() ? 0 : CImGuiManager::GetInstance().GetTexture(ghostIconKey);

		if (ghostTex) {
			dl->AddImage(ghostTex, ghostMin, ghostMax);
		}
		else {
			const char* name = dragged_item->GetName().c_str();
			ImVec2 tSz = ImGui::CalcTextSize(name);
			dl->AddText(ImVec2(mousePos.x - tSz.x * 0.5f, mousePos.y - tSz.y * 0.5f),
			            IM_COL32(30, 30, 30, 255), name);
		}
	}

	// 마우스 버튼 놓으면 드래그 종료
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && is_dragging && dragged_item) {

		// 인벤토리 창 밖에서 놓았는지 확인
		ImGuiWindow* win = ImGui::FindWindowByName("##Inventory");
		bool droppedOutside = true;
		if (win) {
			ImVec2 mp = ImGui::GetMousePos();
			droppedOutside = !(mp.x >= win->Pos.x && mp.x <= win->Pos.x + win->Size.x &&
			                   mp.y >= win->Pos.y && mp.y <= win->Pos.y + win->Size.y);
		}

		if (droppedOutside) {

			// 퀵슬롯 위에 드롭한 경우: 등록만 하고 인벤토리에서는 제거하지 않음
			bool handled_by_quickslot = false;
			if (quick_slot) {
				ImVec2 mp = ImGui::GetMousePos();
				handled_by_quickslot = quick_slot->TryDropOnSlot(dragged_item, mp);
			}

			if (!handled_by_quickslot) {
				if (g_is_single) {
					auto item = std::find_if(items.begin(), items.end(), [this](const std::pair<uint32, std::shared_ptr<CItem>>& pair) {
						return pair.second.get() == dragged_item;
						});

					if (item != items.end()) {
						if (item->second->GetItemType() == ITEM_TYPE::TREASURE)
							current_weight -= item->second->GetWeight();

						if (on_drop_callback)
							on_drop_callback(item->second);

						if (quick_slot)
							quick_slot->OnItemRemovedFromInventory(item->first);

						items.erase(item);
					}
				}
				else {
					auto item = std::find_if(items.begin(), items.end(), [this](const std::pair<uint32, std::shared_ptr<CItem>>& pair) {
						return pair.second.get() == dragged_item;
						});

					if (item != items.end()) {
						C_DropItem dropPkt;
						dropPkt.player_id = owner.lock()->GetID();
						dropPkt.inventory_id = item->first;
						dropPkt.item_type = item->second->GetItemType();
						dropPkt.scene_type = owner.lock()->GetCurrentSceneType();

						if (owner.lock()->GetSession()) {
							auto sendBuffer = MAKE_SEND_BUFFER(dropPkt);
							owner.lock()->GetSession()->DoSend(sendBuffer);
						}
					}
				}
			}
		}

		is_dragging  = false;
		dragged_item = nullptr;
	}
}

void CInventory::DrawTitleBar(float winW, float titleH)
{
	float scale = G_RATIO_Y;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0)); // 투명 - 배경 직접 그림
	if (ImGui::BeginChild("##TitleBar", ImVec2(winW, titleH), false)) {

		// 위쪽 모서리만 둥글게 배경 직접 그리기
		ImDrawList* dl   = ImGui::GetWindowDrawList();
		ImVec2      wpos = ImGui::GetWindowPos();
		ImVec2      wsz  = ImGui::GetWindowSize();
		dl->AddRectFilled(wpos, ImVec2(wpos.x + wsz.x, wpos.y + wsz.y),
		                  IM_COL32(115, 115, 115, 64), 20.0f * scale, ImDrawFlags_RoundCornersTop);

		ImGui::SetWindowFontScale(scale);

		// "Inventory" 텍스트 가운데 정렬 (Bold + 형광 노란색)
		const char* title = "Inventory";

		if (CImGuiManager::bold_font) 
			ImGui::PushFont(CImGuiManager::bold_font);

		float textW = ImGui::CalcTextSize(title).x;
		ImGui::SetCursorPos(ImVec2((winW - textW) * 0.5f, (titleH - ImGui::GetTextLineHeight()) * 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // 형광 노란색
		ImGui::Text(title);
		ImGui::PopStyleColor();

		if (CImGuiManager::bold_font) 
			ImGui::PopFont();

		// 우상단 빨간 X 버튼
		float btnSize = 26.0f * scale;
		ImGui::SetCursorPos(ImVec2(winW - btnSize - 15.0f * scale, (titleH - btnSize) * 0.5f));
		ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.85f, 0.10f, 0.10f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.25f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.00f, 0.00f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);

		if (ImGui::Button("X##close", ImVec2(btnSize, btnSize)))
			is_open = false;

		ImGui::PopStyleVar();
		ImGui::PopStyleColor(3);
		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor(); // ChildBg
}

void CInventory::DrawTabBar()
{
	float scale  = G_RATIO_Y;
	float winW   = ImGui::GetWindowWidth();
	float gap    = 4.0f * scale;
	float tabW   = (winW - gap * 3) / 4.0f;
	float tabH   = 30.0f * scale;

	struct TabInfo { const char* label; ITEM_TYPE type; };
	TabInfo tabs[4] = {
		{ (const char*)u8"장비",      ITEM_TYPE::EQUIPMENT  },
		{ (const char*)u8"소비",      ITEM_TYPE::CONSUMABLE },
		{ (const char*)u8"기타",      ITEM_TYPE::ETC        },
		{ (const char*)u8"보물",      ITEM_TYPE::TREASURE   },
	};

	// 탭 전체 영역 시작점 기록 (외곽선용)
	ImVec2 tabBarStart = ImGui::GetCursorScreenPos();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f * scale);

	for (int i = 0; i < 4; i++) {

		bool isActive = (active_tab == tabs[i].type);

		if (isActive) {
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.85f, 0.15f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.10f, 0.40f, 0.80f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.40f, 0.80f, 1.0f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f, 0.50f, 0.85f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.60f, 1.00f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f, 0.40f, 0.80f, 1.0f));
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

		ImGui::PushFont(CImGuiManager::bold_font);

		if (ImGui::Button(tabs[i].label, ImVec2(tabW, tabH)))
			active_tab = tabs[i].type;

		ImGui::PopStyleColor(4);
		ImGui::PopFont();

		if (i < 3) 
			ImGui::SameLine();
	}

	ImGui::PopStyleVar(2);

	// 선택된 탭의 아이템 렌더링
	if (active_tab == ITEM_TYPE::TREASURE)
		DrawItemTable(active_tab);
	else
		DrawItemGrid(active_tab);
}

void CInventory::DrawItemGrid(ITEM_TYPE type)
{
	float scale = G_RATIO_Y;

	std::vector<CItem*> filtered;
	for (auto& [id, item] : items) {
		if (item->GetItemType() == type)
			filtered.push_back(item.get());
	}
	std::sort(filtered.begin(), filtered.end(), [](CItem* a, CItem* b) {
		return a->GetInventoryID() < b->GetInventoryID();
	});

	int displayCount = (int)filtered.size();

	const int cols    = 4;
	float     bottomH = 40.0f * scale;
	float     pad     = 4.0f * scale;
	float     rounding = 6.0f * scale;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

	if (ImGui::BeginChild("##ItemScroll", ImVec2(0, -bottomH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {

		ImGui::SetWindowFontScale(scale);

		float avail  = ImGui::GetContentRegionAvail().x;
		float cellSz = (avail - pad * (cols + 1)) / cols;

		int minSlots = cols * 5;
		int total    = std::max(displayCount, minSlots);
		int rows     = (total + cols - 1) / cols;

		ImGui::Dummy(ImVec2(0.0f, pad * 2)); // 상단 여백

		for (int row = 0; row < rows; row++) {

			ImGui::SetCursorPosX(pad);
			for (int col = 0; col < cols; col++) {
				int idx = row * cols + col;

				ImVec2      cellMin = ImGui::GetCursorScreenPos();
				ImVec2      cellMax = ImVec2(cellMin.x + cellSz, cellMin.y + cellSz);
				ImDrawList* dl      = ImGui::GetWindowDrawList();

				dl->AddRectFilled(cellMin, cellMax, IM_COL32(210, 210, 215, 255), rounding);
				dl->AddRect(cellMin, cellMax,       IM_COL32(170, 170, 175, 255), rounding);

				CItem* displayItem = nullptr;

				if (idx < (int)filtered.size())
					displayItem = filtered[idx];

				if (displayItem) {

					const std::string& iconKey = displayItem->GetIconPath();
					ImTextureID tex = iconKey.empty() ? 0 : CImGuiManager::GetInstance().GetTexture(iconKey);

					if (tex) {
						float pad2 = 4.0f * scale;
						ImVec2 imgMin = ImVec2(cellMin.x + pad2, cellMin.y + pad2);
						ImVec2 imgMax = ImVec2(cellMax.x - pad2, cellMax.y - pad2);
						dl->AddImage(tex, imgMin, imgMax);
					}
					else {
						const char* placeholder = displayItem->GetName().c_str();
						ImVec2 tSz  = ImGui::CalcTextSize(placeholder);
						ImVec2 tPos = ImVec2(cellMin.x + (cellSz - tSz.x) * 0.5f,
						                     cellMin.y + (cellSz - tSz.y) * 0.5f);
						dl->AddText(tPos, IM_COL32(100, 100, 100, 255), placeholder);
					}

				}

				// InvisibleButton: 클릭/드래그 감지 + 창 이동 방지
				char btnId[32];
				snprintf(btnId, sizeof(btnId), "##grid_slot_%d", idx);
				ImGui::InvisibleButton(btnId, ImVec2(cellSz, cellSz));

				if (displayItem) {
					if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f)) {
						is_dragging  = true;
						dragged_item = displayItem;
					}

					if (!is_dragging && ImGui::IsItemHovered())
						DrawItemTooltip(displayItem);
				}

				if (col < cols - 1)
					ImGui::SameLine(0.0f, pad);
			}
			ImGui::Dummy(ImVec2(0.0f, pad)); // 행 간격
		}

		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void CInventory::DrawItemTable(ITEM_TYPE type)
{
	float scale = G_RATIO_Y;

	// 현재 탭에 해당하는 아이템 필터링
	std::vector<CItem*> filtered;
	for (auto& [id, item] : items) {
		if (item->GetItemType() == type)
			filtered.push_back(item.get());
	}
	std::sort(filtered.begin(), filtered.end(), [](CItem* a, CItem* b) {
		return a->GetInventoryID() < b->GetInventoryID();
	});

	float bottomH = 40.0f * scale;
	float rowH    = 70.0f * scale;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 흰색 배경

	if (ImGui::BeginChild("##ItemScroll", ImVec2(0, -bottomH), false, ImGuiWindowFlags_AlwaysVerticalScrollbar)) {

		ImGui::SetWindowFontScale(scale);

		ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingFixedFit;

		ImGui::PushStyleColor(ImGuiCol_TableRowBg,    ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 흰색
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // 흰색
		ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // 검정 텍스트

		if (ImGui::BeginTable("##ItemTable", 3, tableFlags)) {

			float imgColW = rowH; // 이미지 슬롯은 정사각형
			ImGui::TableSetupColumn("##img",    ImGuiTableColumnFlags_WidthFixed,  imgColW);
			ImGui::TableSetupColumn("##name",   ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##weight", ImGuiTableColumnFlags_WidthFixed,  70.0f * scale);

			int minRows   = 50;
			int totalRows = std::max((int)filtered.size(), minRows);

			float slotPad = 1.5f * scale;  // 슬롯 내부 여백
			float rounding = 6.f * scale; // 슬롯 모서리 둥글기

			for (int i = 0; i < totalRows; i++) {
				ImGui::TableNextRow(ImGuiTableRowFlags_None, rowH);

				// 이미지 셀 - 연한 회색 둥근 슬롯 박스
				ImGui::TableSetColumnIndex(0);
				{
					ImVec2 cellMin = ImGui::GetCursorScreenPos();
					float  slotSz  = rowH - slotPad * 2.0f;
					float  imgOffsetX = 2.5f * scale; // 이미지 칸 오른쪽으로 살짝 이동
					ImVec2 slotMin = ImVec2(cellMin.x + slotPad + imgOffsetX, cellMin.y + slotPad);
					ImVec2 slotMax = ImVec2(slotMin.x + slotSz,  slotMin.y + slotSz);

					ImDrawList* dl = ImGui::GetWindowDrawList();
					// 보물 등급별 테두리 색상
					ImU32 borderColor = IM_COL32(170, 170, 175, 255); // 기본 회색
					if (i < (int)filtered.size()) {
						CTreasure* treasure = static_cast<CTreasure*>(filtered[i]);
						switch (treasure->GetGrade()) {
						case TREASURE_GRADE::COMMON:   borderColor = IM_COL32(160, 160, 160, 255); break; // 회색
						case TREASURE_GRADE::UNCOMMON: borderColor = IM_COL32( 80, 200,  80, 255); break; // 초록
						case TREASURE_GRADE::RARE:     borderColor = IM_COL32( 80, 140, 255, 255); break; // 파랑
						case TREASURE_GRADE::EPIC:     borderColor = IM_COL32(180,  80, 255, 255); break; // 보라
						case TREASURE_GRADE::LEGENDARY:borderColor = IM_COL32(255, 165,   0, 255); break; // 주황/금
						}
					}

					dl->AddRectFilled(slotMin, slotMax, IM_COL32(210, 210, 215, 255), rounding); // 연한 회색 채우기
					dl->AddRect(slotMin, slotMax, borderColor, rounding, 0, 2.0f * scale);       // 등급별 테두리

					if (i < (int)filtered.size()) {

						// TODO: 실제 텍스처 로드 후 ImGui::SetCursorScreenPos(slotMin) + ImGui::Image()로 교체
						ImVec2 textPos = ImVec2(slotMin.x + slotSz * 0.5f - ImGui::CalcTextSize("[img]").x * 0.5f,
						                        slotMin.y + slotSz * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
						dl->AddText(textPos, IM_COL32(100, 100, 100, 255), "[img]");
					}

					// InvisibleButton: 클릭/드래그 감지 + 창 이동 방지
					char btnId[32];
					snprintf(btnId, sizeof(btnId), "##table_slot_%d", i);
					ImGui::InvisibleButton(btnId, ImVec2(imgColW, rowH));

					if (i < (int)filtered.size()) {
						if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f)) {
							is_dragging  = true;
							dragged_item = filtered[i];
						}
						if (!is_dragging && ImGui::IsItemHovered() && type != ITEM_TYPE::TREASURE)
							DrawItemTooltip(filtered[i]);
					}
				}

				// 아이템 이름 셀
				ImGui::TableSetColumnIndex(1);
				{
					ImVec2      cellMin = ImGui::GetCursorScreenPos();
					float       cellW   = ImGui::GetContentRegionAvail().x;
					float       boxOffsetX = 2.5f * scale;
					ImVec2      boxMin  = ImVec2(cellMin.x + slotPad + boxOffsetX, cellMin.y + slotPad);
					ImVec2      boxMax  = ImVec2(cellMin.x + cellW - slotPad, cellMin.y + rowH - slotPad);
					ImDrawList* dl      = ImGui::GetWindowDrawList();
					dl->AddRectFilled(boxMin, boxMax, IM_COL32(210, 210, 215, 255), rounding);
					dl->AddRect(boxMin, boxMax,       IM_COL32(170, 170, 175, 255), rounding);

					if (i < (int)filtered.size()) {
						std::string name    = filtered[i]->GetName();
						ImFont*     font    = ImGui::GetFont();
						float       baseSz  = ImGui::GetFontSize();
						float       innerW  = (boxMax.x - boxMin.x) - slotPad * 4.0f;
						float       textW   = font->CalcTextSizeA(baseSz, FLT_MAX, 0.0f, name.c_str()).x;
						float       fontSize = (textW > innerW) ? baseSz * (innerW / textW) : baseSz;
						float       scaledW = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, name.c_str()).x;
						float       textX   = boxMin.x + (boxMax.x - boxMin.x - scaledW) * 0.5f;
						float       textY   = boxMin.y + (boxMax.y - boxMin.y - fontSize) * 0.5f;
						dl->AddText(font, fontSize, ImVec2(textX, textY), IM_COL32(0, 0, 0, 255), name.c_str());
					}
					ImGui::Dummy(ImVec2(cellW, rowH));
				}

				// 무게 셀
				ImGui::TableSetColumnIndex(2);
				{
					ImVec2      cellMin = ImGui::GetCursorScreenPos();
					float       cellW   = ImGui::GetContentRegionAvail().x;
					ImVec2      boxMin  = ImVec2(cellMin.x + slotPad, cellMin.y + slotPad);
					ImVec2      boxMax  = ImVec2(cellMin.x + cellW - slotPad, cellMin.y + rowH - slotPad);
					ImDrawList* dl      = ImGui::GetWindowDrawList();
					ImFont*     font    = ImGui::GetFont();
					float       baseSz  = ImGui::GetFontSize();
					dl->AddRectFilled(boxMin, boxMax, IM_COL32(210, 210, 215, 255), rounding);
					dl->AddRect(boxMin, boxMax,       IM_COL32(170, 170, 175, 255), rounding);

					if (i < (int)filtered.size()) {
						char  buf[32];
						snprintf(buf, sizeof(buf), "%.0f un", filtered[i]->GetWeight());
						float       textW = font->CalcTextSizeA(baseSz, FLT_MAX, 0.0f, buf).x;
						float       textX = boxMin.x + (boxMax.x - boxMin.x - textW) * 0.5f;
						float       textY = boxMin.y + (boxMax.y - boxMin.y - baseSz) * 0.5f;
						dl->AddText(font, baseSz, ImVec2(textX, textY), IM_COL32(0, 0, 0, 255), buf);
					}
					ImGui::Dummy(ImVec2(cellW, rowH));
				}
			}

			ImGui::EndTable();
		}

		ImGui::PopStyleColor(3);
		ImGui::SetWindowFontScale(1.0f);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor(); // ChildBg
}

void CInventory::DrawItemTooltip(CItem* item)
{
	if (!item)
		return;

	float scale    = G_RATIO_Y;
	float imgSz    = 70.0f * scale;
	float padX     = 12.0f * scale;
	float padY     = 8.0f  * scale;
	float rounding = 6.0f  * scale;
	float descW    = 180.0f * scale;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f * scale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(padX, padY));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.17f, 0.17f, 0.18f, 0.93f));

	if (ImGui::BeginTooltip()) {

		ImGui::SetWindowFontScale(scale);

		// ── 1. 아이템 이름 (가운데 정렬, 볼드) ─────────────────────
		if (CImGuiManager::bold_font)
			ImGui::PushFont(CImGuiManager::bold_font);

		const char* name   = item->GetName().c_str();
		float       totalW = imgSz + padX + descW;
		float       nameW  = ImGui::CalcTextSize(name).x;
		ImGui::SetCursorPosX(padX + (totalW - nameW) * 0.5f);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::Text("%s", name);
		ImGui::PopStyleColor();

		if (CImGuiManager::bold_font)
			ImGui::PopFont();

		// ── 2. 구분선 ────────────────────────────────────────────
		ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.55f, 0.55f, 0.58f, 0.7f));
		ImGui::Separator();
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, padY * 0.3f));

		// ── 3. 이미지 셀 | 설명 텍스트 ────────────────────────────
		ImDrawList* dl = ImGui::GetWindowDrawList();

		// 왼쪽: 이미지 셀
		ImGui::BeginGroup();
		{
			ImVec2 imgMin = ImGui::GetCursorScreenPos();
			ImVec2 imgMax = ImVec2(imgMin.x + imgSz, imgMin.y + imgSz);
			dl->AddRectFilled(imgMin, imgMax, IM_COL32(210, 210, 215, 255), rounding);
			dl->AddRect(imgMin, imgMax, IM_COL32(170, 170, 175, 255), rounding);

			const std::string& iconKey = item->GetIconPath();
			ImTextureID tex = iconKey.empty() ? 0 : CImGuiManager::GetInstance().GetTexture(iconKey);
			if (tex) {
				float   ip   = 4.0f * scale;
				dl->AddImage(tex, ImVec2(imgMin.x + ip, imgMin.y + ip),
				                  ImVec2(imgMax.x - ip, imgMax.y - ip));
			}

			ImGui::Dummy(ImVec2(imgSz, imgSz));
		}
		ImGui::EndGroup();

		ImGui::SameLine(0.0f, padX);

		// 오른쪽: 타입별 정보
		ImGui::BeginGroup();
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.88f, 0.88f, 1.0f));
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + descW);

			auto* tool = dynamic_cast<CTool*>(item);
			if (tool) {
				// 도구: 설명 표시
				// 도구: 설명 표시
				const std::string& desc = tool->GetDescription();
				if (!desc.empty()) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.78f, 0.88f, 1.0f, 0.92f));
					ImGui::TextWrapped("%s", desc.c_str());
					ImGui::PopStyleColor();

					ImGui::Dummy(ImVec2(0.0f, 3.0f * scale));
					ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.45f, 0.45f, 0.50f, 0.5f));
					ImGui::Separator();
					ImGui::PopStyleColor();
					ImGui::Dummy(ImVec2(0.0f, 4.0f * scale));
				}

				// 내구도 표시
				uint32 cur = tool->GetCurrentDurability();
				uint32 max = tool->GetMaxDurability();
				float  ratio = (max > 0) ? (float)cur / (float)max : 0.0f;

				// 내구도 바 색상 (높으면 초록, 낙으면 빨강)
				ImVec4 barColor = (ratio > 0.5f) ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f)
				                : (ratio > 0.25f) ? ImVec4(0.9f, 0.7f, 0.1f, 1.0f)
				                                 : ImVec4(0.9f, 0.2f, 0.2f, 1.0f);

				// 내구도 레이블 (황금색)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.72f, 0.35f, 1.0f));
				ImGui::TextUnformatted((const char*)u8"내구도");
				ImGui::PopStyleColor();

				// 수치 텍스트 (내구도 바 색상과 동일)
				char durBuf[32];
				snprintf(durBuf, sizeof(durBuf), "%u / %u", cur, max);
				ImGui::PushStyleColor(ImGuiCol_Text, barColor);
				ImGui::TextUnformatted(durBuf);
				ImGui::PopStyleColor();

				// 내구도 바
				ImVec2 barMin = ImGui::GetCursorScreenPos();
				ImVec2 barMax = ImVec2(barMin.x + descW, barMin.y + 10.0f * scale);
				dl->AddRectFilled(barMin, barMax, IM_COL32(80, 80, 85, 255), 3.0f * scale);
				dl->AddRectFilled(barMin,
				                  ImVec2(barMin.x + descW * ratio, barMax.y),
				                  ImGui::ColorConvertFloat4ToU32(barColor), 3.0f * scale);
				dl->AddRect(barMin, barMax, IM_COL32(120, 120, 125, 200), 3.0f * scale);
				ImGui::Dummy(ImVec2(descW, 10.0f * scale));
			}
			else {
				// 무기 / 소비 / 기타 / 보물: description
				const std::string& desc = item->GetDescription();
				if (!desc.empty())
					ImGui::TextWrapped("%s", desc.c_str());
			}

			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
		}
		ImGui::EndGroup();

		ImGui::Dummy(ImVec2(totalW + padX * 2.0f, 0.0f)); // 최소 너비 확보

		ImGui::SetWindowFontScale(1.0f);
		ImGui::EndTooltip();
	}

	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void CInventory::DrawBottomBar()
{
	float scale = G_RATIO_Y;

	auto player = owner.lock();
	if (!player)
		return;

	float bottomH = 40.0f * scale;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0)); // 투명 - 배경 직접 그림
	ImGui::PushStyleColor(ImGuiCol_Text,    ImVec4(0.00f, 0.00f, 0.00f, 1.0f)); // 검정 텍스트

	if (ImGui::BeginChild("##BottomBar", ImVec2(0, bottomH), false)) {

		// 아래쪽 모서리만 둥글게 노란 배경 직접 그리기
		ImDrawList* dl   = ImGui::GetWindowDrawList();
		ImVec2      wpos = ImGui::GetWindowPos();
		ImVec2      wsz  = ImGui::GetWindowSize();
		dl->AddRectFilled(wpos, ImVec2(wpos.x + wsz.x, wpos.y + wsz.y),
		                  IM_COL32(255, 199, 0, 255), 20.0f * scale, ImDrawFlags_RoundCornersBottom);

		ImGui::SetWindowFontScale(scale);

		if (CImGuiManager::bold_font)
			ImGui::PushFont(CImGuiManager::bold_font);

		float barH    = ImGui::GetWindowHeight();
		float textY   = (barH - ImGui::GetTextLineHeight()) * 0.5f;
		float padX    = 10.0f * scale;

		// 소지금 (왼쪽)
		char goldBuf[64];
		snprintf(goldBuf, sizeof(goldBuf), "%llu", player->GetGold());
		std::string goldText = (const char*)u8"소지금: ";
		std::string unit = (const char*)u8"원";
		goldText += goldBuf + unit;
		ImGui::SetCursorPos(ImVec2(padX, textY));
		ImGui::Text("%s", goldText.c_str());

		// 가방 용량 (오른쪽)
		char weightBuf[64];
		snprintf(weightBuf, sizeof(weightBuf), "%u  / %u", current_weight, max_weight);
		std::string weightText = (const char*)u8"가방:  ";
		weightText += weightBuf;
		float textW = ImGui::CalcTextSize(weightText.c_str()).x;
		ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - textW - padX, textY));
		ImGui::Text("%s", weightText.c_str());

		ImGui::SetWindowFontScale(1.0f);

		if (CImGuiManager::bold_font)
			ImGui::PopFont();
	}
	ImGui::EndChild();

	ImGui::PopStyleColor(2);
}
