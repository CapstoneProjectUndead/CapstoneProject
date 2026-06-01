#pragma once
#include <random>
#include <vector>
#include <string>
#include <ImGui/imgui.h>

class CMyPlayer;

// 상점 그리드에 표시되는 판매 항목 하나
struct ShopSlot
{
    int         item_id    = 0;
    uint32      base_price = 0;   // items.json 가격 (변동 기준값)
    uint32      price      = 0;   // 변동 적용된 현재 가격
    int         stock      = 0;
    bool        is_fixed   = false;
    std::string name;             // UI용 캐시
    std::string icon_path;        // UI용 캐시
};

// 플레이어별 상점. 카탈로그/재고/가격을 직접 보유하고 UI를 그린다.
class CShop
{
private:
    CShop();
    CShop(const CShop&) = delete;

public:
    ~CShop();

    static CShop& GetInstance() {
        static CShop instance;
        return instance;
    }

public:
    // 열림/닫힘 상태 (씬에서 입력/커서 처리에 사용)
    void Open();
    void Close();
    bool IsOpen() const { return is_open; }

    // 카탈로그 재구성: 재고 리필 + 랜덤 재선정 + 가격 재변동.
    // 게임 종료 시, 그리고 유료 새로고침 시 호출.
    void Reset();

    // 상점 패널(좌) + 플레이어 인벤토리(우)를 나란히 그린다.
    void DrawStoreUI(std::shared_ptr<CMyPlayer> player);

    // 해당 슬롯에서 살 수 있는 최대 수량 = min(재고, 보유골드 / 가격)
    int  MaxBuyable(const std::shared_ptr<CMyPlayer>& player, int slotIndex) const;

private:
    void EnsureInitialized();
    bool Purchase(const std::shared_ptr<CMyPlayer>& player, int slotIndex, int qty);
    bool RefreshPaid(const std::shared_ptr<CMyPlayer>& player); // 500골드 차감 후 Reset

    uint32 ApplyPriceVariation(uint32 base);

    // UI 헬퍼
    void DrawShopPanel(const std::shared_ptr<CMyPlayer>& player, float x, float y, float w, float h);
    void DrawShopCard(const std::shared_ptr<CMyPlayer>& player, int index, float w, float h);
    void DrawQuantityModal(const std::shared_ptr<CMyPlayer>& player);
    void OpenQtyModal(int slotIndex);

    void CheckHoverSound();
    void PlayClickSound();

private:
    std::vector<ShopSlot> slots;
    bool   initialized = false;
    bool   is_open     = false;

    int    selected_slot  = -1;   // 현재 선택된 카드
    bool   open_qty_modal = false;
    int    qty_modal_slot = -1;
    int    buy_qty        = 1;

    std::mt19937 rng_;

    ImGuiID last_hovered_id = 0;
};
