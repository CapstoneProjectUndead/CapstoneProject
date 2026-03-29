#include "stdafx.h"
#include "Inventory.h"
#include "ImGuiManager.h"
#include "MyPlayer.h"
#include "ServerPacketHandler.h"

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
void CInventory::AddItem(std::shared_ptr<CItem> item)
{
	// 보물만 무게에 영향을 준다.
	if (item->GetItemType() == ITEM_TYPE::TREASURE) {

		// 이미 꽉 찼다면 return!
		if (current_weight >= max_weight)
			return;

		// 이 보물을 추가했을 때 최대 무게를 초과하면 거부
		if (current_weight + item->GetWeight() > max_weight)
			return;

		current_weight += item->GetWeight();
	}

	uint32 id = inventory_id_counter++;
	item->SetInventoryID(id);
	items[id] = std::move(item);
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
	if (it != items.end()) {
		current_weight -= it->second->GetWeight();
		items.erase(it);
	}
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

	ImGui::SetNextWindowPos(ImVec2(screen.x * 0.5f, screen.y * 0.5f), ImGuiCond_Once, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

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

		const char* name = dragged_item->GetName().c_str();
		ImVec2 tSz = ImGui::CalcTextSize(name);
		dl->AddText(ImVec2(mousePos.x - tSz.x * 0.5f, mousePos.y - tSz.y * 0.5f),
		            IM_COL32(30, 30, 30, 255), name);
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

			if (g_is_single) {
				auto item = std::find_if(items.begin(), items.end(), [this](const std::pair<uint32, std::shared_ptr<CItem>>& pair) {
					return pair.second.get() == dragged_item;
					});

				if (item != items.end()) {
					if (item->second->GetItemType() == ITEM_TYPE::TREASURE)
						current_weight -= item->second->GetWeight();

					if (on_drop_callback)
						on_drop_callback(item->second);

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
		int total    = std::max((int)filtered.size(), minSlots);
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

				if (idx < (int)filtered.size()) {

					// TODO: 실제 아이템 이미지 로드 후 ImGui::Image()로 교체
					const char* placeholder = filtered[idx]->GetName().c_str();
					ImVec2 tSz  = ImGui::CalcTextSize(placeholder);
					ImVec2 tPos = ImVec2(cellMin.x + (cellSz - tSz.x) * 0.5f,
					                     cellMin.y + (cellSz - tSz.y) * 0.5f);
					dl->AddText(tPos, IM_COL32(100, 100, 100, 255), placeholder);
				}

				// InvisibleButton: 클릭/드래그 감지 + 창 이동 방지
				char btnId[32];
				snprintf(btnId, sizeof(btnId), "##grid_slot_%d", idx);
				ImGui::InvisibleButton(btnId, ImVec2(cellSz, cellSz));

				if (idx < (int)filtered.size()) {
					if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0f)) {
						is_dragging  = true;
						dragged_item = filtered[idx];
					}
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
					dl->AddRectFilled(slotMin, slotMax, IM_COL32(210, 210, 215, 255), rounding); // 연한 회색 채우기
					dl->AddRect(slotMin, slotMax,       IM_COL32(170, 170, 175, 255), rounding); // 테두리

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
