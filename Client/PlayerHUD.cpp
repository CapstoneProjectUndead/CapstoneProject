#include "stdafx.h"
#include "PlayerHUD.h"
#include "MyPlayer.h"
#include "ImGuiManager.h"
#include "ImGui/imgui.h"

// Player_UI.json(1920x1080 기준) 값을 600 높이 기준으로 환산(×0.5556)한 레이아웃.
// 모든 px는 그릴 때 G_RATIO_Y를 곱해 해상도에 대응한다.
namespace
{
    constexpr float BAR_X      = 28.f;   // 좌상단 기준 가로 여백
    constexpr float BAR_W      = 278.f;
    constexpr float BAR_H      = 22.f;
    constexpr float HP_Y       = 17.f;
    constexpr float STAMINA_Y  = 50.f;

    constexpr float BAG_SIZE   = 83.f;   // 우하단 가방 아이콘
    constexpr float BAG_MARGIN = 28.f;

    float Clamp01(float v)
    {
        if (v < 0.f) return 0.f;
        if (v > 1.f) return 1.f;
        return v;
    }
}

CPlayerHUD::CPlayerHUD(CMyPlayer* owner)
    : owner(owner)
{
}

void CPlayerHUD::Update(float elapsedTime)
{
    // 현재는 즉시 모드로 매 프레임 Draw에서 현재 값을 직접 읽으므로 별도 상태 갱신 불필요.
}

void CPlayerHUD::Draw()
{
    if (!owner)
        return;

    // 사망 시 HUD 숨김 (기존 Player_UI 캔버스 DEAD 토글과 동일 동작)
    if (owner->GetState() == PLAYER_STATE::DEAD)
        return;

    DrawHpBar();
    DrawStaminaBar();
    DrawBag();
    DrawAim();
}

void CPlayerHUD::DrawHpBar()
{
    const float scale = G_RATIO_Y;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    float ratio = 0.f;
    float maxHp = static_cast<float>(owner->GetMaxHp());
    if (maxHp > 0.f)
        ratio = Clamp01(static_cast<float>(owner->GetHp()) / maxHp);

    ImVec2 p0(BAR_X * scale, HP_Y * scale);
    ImVec2 p1(p0.x + BAR_W * scale, p0.y + BAR_H * scale);

    ImU32 bg     = ImGui::GetColorU32(ImVec4(0.538f, 0.332f, 0.332f, 1.f));
    ImU32 fill   = ImGui::GetColorU32(ImVec4(1.000f, 0.182f, 0.182f, 1.f));
    ImU32 border = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.6f));

    dl->AddRectFilled(p0, p1, bg);
    dl->AddRectFilled(p0, ImVec2(p0.x + BAR_W * scale * ratio, p1.y), fill);
    dl->AddRect(p0, p1, border, 0.f, 0, 1.5f * scale);
}

void CPlayerHUD::DrawStaminaBar()
{
    const float scale = G_RATIO_Y;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    float ratio = 0.f;
    float maxStamina = static_cast<float>(owner->GetMaxStamina());
    if (maxStamina > 0.f)
        ratio = Clamp01(static_cast<float>(owner->GetStamina()) / maxStamina);

    ImVec2 p0(BAR_X * scale, STAMINA_Y * scale);
    ImVec2 p1(p0.x + BAR_W * scale, p0.y + BAR_H * scale);

    ImU32 bg     = ImGui::GetColorU32(ImVec4(0.475f, 0.431f, 0.000f, 1.f));
    ImU32 fill   = ImGui::GetColorU32(ImVec4(1.000f, 0.919f, 0.089f, 1.f));
    ImU32 border = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.6f));

    dl->AddRectFilled(p0, p1, bg);
    dl->AddRectFilled(p0, ImVec2(p0.x + BAR_W * scale * ratio, p1.y), fill);
    dl->AddRect(p0, p1, border, 0.f, 0, 1.5f * scale);
}

void CPlayerHUD::DrawBag()
{
    ImTextureID tex = CImGuiManager::GetInstance().GetTexture("backpack");
    if (!tex)
        return;

    const float scale = G_RATIO_Y;
    ImGuiIO& io = ImGui::GetIO();

    float size   = BAG_SIZE * scale;
    float margin = BAG_MARGIN * scale;

    // 우하단 앵커
    ImVec2 p1(io.DisplaySize.x - margin, io.DisplaySize.y - margin);
    ImVec2 p0(p1.x - size, p1.y - size);

    ImGui::GetBackgroundDrawList()->AddImage(tex, p0, p1);
}

void CPlayerHUD::DrawAim()
{
    const float scale = G_RATIO_Y;
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    ImVec2 c(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    float r = 2.5f * scale;

    // 흰 점 + 얇은 검은 외곽선 (기존 aim_ui 대체 — 단순 조준점)
    dl->AddCircleFilled(c, r, ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.9f)));
    dl->AddCircle(c, r, ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.6f)), 0, 1.5f * scale);
}
