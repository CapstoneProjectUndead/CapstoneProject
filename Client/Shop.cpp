#include "stdafx.h"
#include "Shop.h"
#include "SoundManager.h"
#include "ImGuiManager.h"
#include "ItemFactory.h"
#include "Item.h"
#include "MyPlayer.h"
#include "Inventory.h"
#include "ServerSession.h"
#include "ServerPacketHandler.h"

#include <algorithm>
#include <cmath>
#include <iterator>

#undef min
#undef max

// ---- 카탈로그 설정 (아이템 추가 시 여기서 수정) ----
// 고정 판매 아이템 (항상 판매), 각 재고 3개:
//   1=삽, 5=도끼, 9=곡괭이, 14=야구방망이, 16=유령퇴치스프레이
static const int   FIXED_ITEMS[]      = { 1, 5, 9, 14, 16 };

// 랜덤 장비 풀 (1개 선정), 재고 1개 (아이콘 없는 아이템 제외):
//   15=뿅망치, 17=마법지팡이, 19=장난감칼
static const int   RANDOM_EQUIP_POOL[] = { 15, 17, 19 };

// 랜덤 음식 id 범위 (item_type == 음식); 아이콘 있는 id 중 몇 개를 중복 없이 선정
static constexpr int FOOD_ID_MIN = 20;
static constexpr int FOOD_ID_MAX = 45;

static constexpr int    FIXED_STOCK      = 3;
static constexpr int    BAT_ITEM_ID      = 14;   // baseball bat: fixed item, stock 1
static constexpr int    RANDOM_STOCK_MIN = 1;
static constexpr int    RANDOM_STOCK_MAX = 3;
static constexpr int    RANDOM_FOOD_COUNT = 4;
static constexpr uint32 REFRESH_COST     = 500;

CShop::CShop()
    : rng_(std::random_device{}())
{
}

CShop::~CShop()
{
}

void CShop::Open()
{
    EnsureInitialized();
    is_open       = true;
    selected_slot = -1;
}

void CShop::Close()
{
    is_open        = false;
    open_qty_modal = false;
}

void CShop::EnsureInitialized()
{
    if (!initialized) {
        Reset();
        initialized = true;
    }
}

uint32 CShop::ApplyPriceVariation(uint32 base)
{
    if (base == 0)
        return 0;

    // 기준가의 -50% ~ +100%, 10단위 반올림, 최소 10
    std::uniform_real_distribution<float> dist(0.5f, 2.0f);
    float  varied  = base * dist(rng_);
    uint32 rounded = static_cast<uint32>(std::llround(varied / 10.0) * 10);
    return (rounded < 10) ? 10u : rounded;
}

void CShop::Reset()
{
    slots.clear();

    auto addSlot = [&](int id, int stock, bool fixed) {
        const ItemData* meta = ItemFactory::GetItemData(id);
        if (!meta)
            return;

        ShopSlot s;
        s.item_id    = id;
        s.base_price = meta->price;
        s.price      = ApplyPriceVariation(meta->price);
        s.stock      = stock;
        s.is_fixed   = fixed;
        s.name       = meta->item_name;
        s.icon_path  = meta->icon_path;
        slots.push_back(std::move(s));
    };

    // 고정 아이템
    std::uniform_int_distribution<int> stockDist(RANDOM_STOCK_MIN, RANDOM_STOCK_MAX);
    auto randomStock = [&]() { return stockDist(rng_); };

    for (int id : FIXED_ITEMS)
        addSlot(id, (id == BAT_ITEM_ID) ? 1 : FIXED_STOCK, true);

    // 랜덤 장비 (1개)
    {
        std::vector<int> pool(std::begin(RANDOM_EQUIP_POOL), std::end(RANDOM_EQUIP_POOL));
        std::shuffle(pool.begin(), pool.end(), rng_);
        if (!pool.empty())
            addSlot(pool[0], randomStock(), false);
    }

    // 랜덤 음식 (아이콘 있는 것, 중복 없이)
    {
        std::vector<int> pool;
        for (int id = FOOD_ID_MIN; id <= FOOD_ID_MAX; ++id) {
            const ItemData* m = ItemFactory::GetItemData(id);
            if (m && !m->icon_path.empty())
                pool.push_back(id);
        }
        std::shuffle(pool.begin(), pool.end(), rng_);
        int n = std::min(static_cast<int>(pool.size()), RANDOM_FOOD_COUNT);
        for (int i = 0; i < n; ++i)
            addSlot(pool[i], randomStock(), false);
    }

    selected_slot = -1;
}

int CShop::MaxBuyable(const std::shared_ptr<CMyPlayer>& player, int slotIndex) const
{
    if (!player)
        return 0;

    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size()))
        return 0;

    const ShopSlot& s = slots[slotIndex];
    if (s.price == 0 || s.stock <= 0)
        return 0;

    uint64 gold       = player->GetGold();
    int    affordable = static_cast<int>(gold / s.price);
    return std::min(s.stock, affordable);
}

bool CShop::Purchase(const std::shared_ptr<CMyPlayer>& player, int slotIndex, int qty)
{
    if (!player)
        return false;

    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size()))
        return false;

    ShopSlot& s      = slots[slotIndex];
    int       maxBuy = MaxBuyable(player, slotIndex);
    if (qty <= 0 || qty > maxBuy)
        return false;

    if (g_is_single) {
        auto inventory = player->GetInventory();
        if (!inventory)
            return false;

        int bought = 0;
        for (int i = 0; i < qty; ++i) {
            auto item = ItemFactory::Create(s.item_id);
            if (!item)
                break;
            if (!inventory->AddItem(item))   // 인벤토리 거부 (예: 가득 참)
                break;

            ++bought;
        }

        if (bought <= 0)
            return false;

        uint32 total = s.price * static_cast<uint32>(bought);
        player->SetGold(player->GetGold() - total);
        s.stock -= bought;
    }
    else {
        // (멀티): 클라에서 골드/재고를 먼저 차감하고 서버에 구매를 요청한다.
        // 아이템은 서버 응답(S_AddItemList)을 받아 인벤토리에 추가한다.
        auto session = player->GetSession();
        if (!session)
            return false;

        uint32 total = s.price * static_cast<uint32>(qty);
        player->SetGold(player->GetGold() - total);
        s.stock -= qty;

        C_BuyItem buyPkt;
        buyPkt.player_id  = player->GetID();
        buyPkt.item_id    = static_cast<uint16>(s.item_id);
        buyPkt.qty        = static_cast<uint16>(qty);
        buyPkt.unit_price = s.price;
        buyPkt.scene_type = SCENE_TYPE::LOBBY;

        auto sendBuffer = MAKE_SEND_BUFFER(buyPkt);
        session->DoSend(sendBuffer);
    }

    return true;
}

bool CShop::RefreshPaid(const std::shared_ptr<CMyPlayer>& player)
{
    if (!player)
        return false;
    if (player->GetGold() < REFRESH_COST)
        return false;

    if (g_is_single) {
        player->SetGold(player->GetGold() - REFRESH_COST);
        Reset();
    }
    else{
        // (멀티): 서버에 코인 차감 통지 
        auto session = player->GetSession();
        if (!session)
            return false;

        C_RefreshStore freshPkt;
        freshPkt.player_id = player->GetID();
        freshPkt.scene_type = player->GetCurrentSceneType();
        freshPkt.spent_coin = REFRESH_COST;
        auto sendBuffer = MAKE_SEND_BUFFER(freshPkt);
        session->DoSend(sendBuffer);
    }

    return true;
}

void CShop::OpenQtyModal(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= static_cast<int>(slots.size()))
        return;

    selected_slot  = slotIndex;
    qty_modal_slot = slotIndex;
    buy_qty        = 1;
    open_qty_modal = true;
}

// ========================================================
// UI
// ========================================================
void CShop::DrawStoreUI(std::shared_ptr<CMyPlayer> player)
{
    if (!player || !is_open)
        return;

    EnsureInitialized();

    const float scale  = G_RATIO_Y;
    ImVec2      screen = ImGui::GetIO().DisplaySize;

    const float invW   = 348.f * scale;   // CInventory 창 크기와 동일
    const float invH   = 460.f * scale;
    const float gap    = 16.f  * scale;
    const float margin = 20.f  * scale;
    const float shopH  = invH;            // 높이 맞춤

    // 인벤토리(우)가 잘리지 않도록 상점 폭을 화면 여유에 맞춰 동적 계산.
    // 카드 크기는 이 폭에서 파생되므로 폭이 줄면 카드도 함께 작아진다.
    float shopW = (screen.x - margin * 2.f) - gap - invW;
    shopW = std::min(shopW, 760.f * scale);            // 상한
    if (shopW < 320.f * scale) shopW = 320.f * scale;  // 하한

    const float groupW = shopW + gap + invW;
    float startX = (screen.x - groupW) * 0.5f;
    float startY = (screen.y - shopH) * 0.5f;
    if (startX < 0.f) startX = 0.f;
    if (startY < 0.f) startY = 0.f;

    // 좌: 상점 패널
    DrawShopPanel(player, startX, startY, shopW, shopH);

    // 우: 인벤토리 (강제로 열고 상점 옆에 배치)
    float invX = startX + shopW + gap;
    float invY = startY + (shopH - invH) * 0.5f;
    if (auto inventory = player->GetInventory()) {
        inventory->SetOpen(true);
        inventory->SetPositionOverride(invX, invY);
        inventory->Draw(true);
    }

    // 최상단: 수량 모달
    DrawQuantityModal(player);
}

void CShop::DrawShopPanel(const std::shared_ptr<CMyPlayer>& player, float x, float y, float w, float h)
{
    const float scale    = G_RATIO_Y;
    ImFont*     boldFont = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(w, h), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.42f, 0.42f, 0.44f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14.f * scale, 12.f * scale));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
        | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##shop_panel", nullptr, flags)) {

        // 우상단 닫기(X)
        float xBtn = 26.f * scale;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - xBtn);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.80f, 0.20f, 0.20f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.92f, 0.32f, 0.32f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.65f, 0.12f, 0.12f, 1.f));
        if (ImGui::Button("X", ImVec2(xBtn, xBtn))) {
            Close();
        }
        ImGui::PopStyleColor(3);

        // 스크롤 카드 그리드 (3열, 약 2행 표시)
        const float footerH = 62.f * scale;
        float       gridH   = ImGui::GetContentRegionAvail().y - footerH;
        if (gridH < 50.f * scale) gridH = 50.f * scale;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.80f, 0.82f, 0.84f, 1.f));
        ImGui::BeginChild("##shop_grid", ImVec2(0, gridH), true);
        {
            const int   cols    = 3;
            const float cellGap = 12.f * scale;
            float       innerW  = ImGui::GetContentRegionAvail().x;
            float       cardW   = (innerW - cellGap * (cols - 1)) / cols;
            float       cardH   = cardW * 0.80f;

            for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
                DrawShopCard(player, i, cardW, cardH);
                if ((i % cols) != (cols - 1))
                    ImGui::SameLine(0, cellGap);
                else
                    ImGui::Dummy(ImVec2(0, cellGap));
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // ---- 하단: 새로고침(좌) + 구입(우) ----
        ImGui::PushFont(boldFont);

        // 버튼 행을 살짝 아래로
        ImGui::Dummy(ImVec2(0, 10.f * scale));

        float btnH = 50.f * scale;

        // 새로고침 = 이미지 버튼 + 옆에 500원 표시
        bool canRefresh = (player->GetGold() >= REFRESH_COST);
        if (!canRefresh) ImGui::BeginDisabled();
        ImTextureID refreshTex = CImGuiManager::GetInstance().GetTexture("refresh");
        // 버튼 배경 투명 처리 (아이콘만 보이게)
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.f, 0.f, 0.f, 0.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.f, 1.f, 1.f, 0.15f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.f, 1.f, 1.f, 0.25f));
        bool refreshClicked = refreshTex
            ? ImGui::ImageButton("##refresh_btn", refreshTex, ImVec2(btnH, btnH))
            : ImGui::Button("R", ImVec2(btnH, btnH));
        ImGui::PopStyleColor(3);

        if (refreshClicked) {
            if (RefreshPaid(player))
                PlayClickSound();
        }

        if (!canRefresh) ImGui::EndDisabled();
        CheckHoverSound();

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::SetWindowFontScale(1.0f * scale);
        ImGui::Text("500\xEC\x9B\x90");   // 500원
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SameLine();
        float buyW  = 175.f * scale;
        float avail = ImGui::GetContentRegionAvail().x;
        if (avail > buyW)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - buyW);

        bool canBuy = (selected_slot >= 0 && selected_slot < static_cast<int>(slots.size())
            && slots[selected_slot].stock > 0);
        // 구매 버튼 = 주황색
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.95f, 0.55f, 0.15f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.65f, 0.25f, 1.f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.80f, 0.45f, 0.10f, 1.f));
        if (!canBuy) ImGui::BeginDisabled();
        ImGui::SetWindowFontScale(1.0f * scale);
        if (ImGui::Button("\xEA\xB5\xAC\xEB\xA7\xA4", ImVec2(buyW, btnH))) {
            OpenQtyModal(selected_slot);
        }
        ImGui::SetWindowFontScale(1.0f);
        if (!canBuy) ImGui::EndDisabled();
        ImGui::PopStyleColor(3);
        CheckHoverSound();

        ImGui::PopFont();
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(1);
}

void CShop::DrawShopCard(const std::shared_ptr<CMyPlayer>& player, int index, float w, float h)
{
    const float scale = G_RATIO_Y;
    ShopSlot&   s     = slots[index];

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2      p0 = ImGui::GetCursorScreenPos();
    ImVec2      p1 = ImVec2(p0.x + w, p0.y + h);

    bool soldOut  = (s.stock <= 0);
    bool selected = (selected_slot == index);

    const float stripH = 26.f * scale;
    ImVec2      imgMax = ImVec2(p1.x, p1.y - stripH);

    // 배경: 이미지 영역(파란빛) + 초록 정보 띠
    ImU32 imgBg = soldOut ? IM_COL32(120, 120, 128, 255) : IM_COL32(150, 165, 220, 255);
    dl->AddRectFilled(p0, imgMax, imgBg, 4.f * scale);
    dl->AddRectFilled(ImVec2(p0.x, imgMax.y), p1, IM_COL32(86, 150, 70, 255), 4.f * scale);

    // 아이템 이미지 (정사각형, 이미지 영역 중앙)
    if (!s.icon_path.empty()) {
        ImTextureID tex = CImGuiManager::GetInstance().GetTexture(s.icon_path);
        if (tex) {
            float pad    = 8.f * scale;
            float availW = (imgMax.x - p0.x) - pad * 2.f;
            float availH = (imgMax.y - p0.y) - pad * 2.f;
            float side   = std::min(availW, availH);
            ImVec2 c     = ImVec2((p0.x + imgMax.x) * 0.5f, (p0.y + imgMax.y) * 0.5f);
            ImVec2 iMin  = ImVec2(c.x - side * 0.5f, c.y - side * 0.5f);
            ImVec2 iMax  = ImVec2(c.x + side * 0.5f, c.y + side * 0.5f);
            ImU32  tint  = soldOut ? IM_COL32(255, 255, 255, 90) : IM_COL32(255, 255, 255, 255);
            dl->AddImage(tex, iMin, iMax, ImVec2(0, 0), ImVec2(1, 1), tint);
        }
    }

    // 품절 시 "Sold Out" 이미지 오버레이
    if (soldOut) {
        ImTextureID soldTex = CImGuiManager::GetInstance().GetTexture("sold_out");
        if (soldTex) {
            float  pad  = 6.f * scale;
            ImVec2 sMin = ImVec2(p0.x + pad, p0.y + pad);
            ImVec2 sMax = ImVec2(imgMax.x - pad, imgMax.y - pad);
            dl->AddImage(soldTex, sMin, sMax);
        }
    }

    // 정보 띠: 가격(좌) / 재고(우)
    ImFont* font = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();
    float   fpx  = 13.f * scale;
    char    priceBuf[48];
    char    stockBuf[48];
    snprintf(priceBuf, sizeof(priceBuf), "\xEA\xB0\x80\xEA\xB2\xA9: %u", s.price); // 가격
    snprintf(stockBuf, sizeof(stockBuf), "\xEC\x9E\xAC\xEA\xB3\xA0: %d", s.stock); // 재고
    ImU32 txtCol = IM_COL32(255, 255, 255, 255);
    float ty     = imgMax.y + (stripH - fpx) * 0.5f;
    dl->AddText(font, fpx, ImVec2(p0.x + 6.f * scale, ty), txtCol, priceBuf);
    float sWidth = font->CalcTextSizeA(fpx, FLT_MAX, 0.f, stockBuf).x;
    dl->AddText(font, fpx, ImVec2(p1.x - sWidth - 6.f * scale, ty), txtCol, stockBuf);

    // 테두리 (선택 시 금색)
    if (selected)
        dl->AddRect(p0, p1, IM_COL32(255, 210, 80, 255), 4.f * scale, 0, 3.f * scale);
    else
        dl->AddRect(p0, p1, IM_COL32(60, 60, 66, 180), 4.f * scale, 0, 1.f);

    // 상호작용
    ImGui::InvisibleButton(("##card" + std::to_string(index)).c_str(), ImVec2(w, h));
    if (!soldOut) {
        if (ImGui::IsItemHovered()) {
            dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 110), 4.f * scale, 0, 2.f * scale);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                OpenQtyModal(index);
            }
        }
        if (ImGui::IsItemClicked()) {
            selected_slot = index;
        }
    }
}

void CShop::DrawQuantityModal(const std::shared_ptr<CMyPlayer>& player)
{
    if (!open_qty_modal)
        return;

    int slot = qty_modal_slot;
    if (slot < 0 || slot >= static_cast<int>(slots.size())) {
        open_qty_modal = false;
        return;
    }

    const float scale = G_RATIO_Y;
    ImGuiIO&    io    = ImGui::GetIO();

    // 일반 윈도우 (팝업 스택 의존성 없음); 매 프레임 포커스로 최상단 유지
    ImVec2 center = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowFocus();

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.16f, 0.16f, 0.18f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Text,     ImVec4(0.95f, 0.93f, 0.88f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
        | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
        | ImGuiWindowFlags_AlwaysAutoResize;   // 콘텐츠에 맞춰 자동 크기 (여백 제거)

    if (ImGui::Begin("##buy_qty_win", nullptr, flags)) {
        ImGui::SetWindowFontScale(scale);

        ShopSlot& s      = slots[slot];
        int       maxBuy = MaxBuyable(player, slot);

        ImFont* boldFont = CImGuiManager::bold_font ? CImGuiManager::bold_font : ImGui::GetFont();
        ImGui::PushFont(boldFont);

        ImGui::TextUnformatted(s.name.c_str());
        ImGui::Spacing();

        // 수량을 [1, maxBuy]로 제한
        if (buy_qty < 1) buy_qty = 1;
        if (maxBuy > 0 && buy_qty > maxBuy) buy_qty = maxBuy;

        // 수량 라벨
        ImGui::TextUnformatted("\xEC\x88\x98\xEB\x9F\x89:");
        ImGui::SameLine();
        if (ImGui::Button("-", ImVec2(30.f * scale, 0))) {
            if (buy_qty > 1) --buy_qty;
        }
        ImGui::SameLine();
        ImGui::Text("%d", buy_qty);
        ImGui::SameLine();
        if (ImGui::Button("+", ImVec2(30.f * scale, 0))) {
            if (buy_qty < maxBuy) ++buy_qty;
        }
        ImGui::SameLine();
        ImGui::Text("/ %d", maxBuy);

        // 총액 라벨
        uint32 total = s.price * static_cast<uint32>(buy_qty < 1 ? 0 : buy_qty);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.85f, 0.30f, 1.f));
        ImGui::Text("\xEC\xB4\x9D\xEC\x95\xA1: %u", total);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        bool canBuy = (maxBuy > 0);
        if (!canBuy) ImGui::BeginDisabled();

        // 구매 버튼
        if (ImGui::Button("\xEA\xB5\xAC\xEB\xA7\xA4", ImVec2(130.f * scale, 36.f * scale))) {
            if (Purchase(player, slot, buy_qty))
                PlayClickSound();
            open_qty_modal = false;
        }
        if (!canBuy) ImGui::EndDisabled();

        ImGui::SameLine();
        // 취소 버튼
        if (ImGui::Button("\xEC\xB7\xA8\xEC\x86\x8C", ImVec2(130.f * scale, 36.f * scale))) {
            open_qty_modal = false;
        }

        ImGui::PopFont();
    }
    ImGui::End();

    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(2);
}

void CShop::CheckHoverSound()
{
    if (!ImGui::IsItemHovered())
        return;

    ImGuiID id = ImGui::GetItemID();
    if (id != last_hovered_id) {
        last_hovered_id = id;
        CSoundManager::GetInstance().Play(SOUND_ID::button01a);
    }
}

void CShop::PlayClickSound()
{
    CSoundManager::GetInstance().Play(SOUND_ID::select09);
}
