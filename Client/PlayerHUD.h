#pragma once

class CMyPlayer;

// 플레이어 화면 HUD (HP바, 스태미나바, 가방, 조준점).
// CMyPlayer가 unique_ptr로 소유한다. owner는 비소유 역참조(소유자가 항상 더 오래 삶).
class CPlayerHUD
{
public:
    explicit CPlayerHUD(CMyPlayer* owner);

    // 시간 기반 상태 갱신용 훅 (현재는 비어 있음 — 바 보간/피격 플래시 등 추후 확장)
    void Update(float elapsedTime);

    // ImGui 렌더 (씬 DrawUI에서 호출)
    void Draw();

private:
    void DrawHpBar();
    void DrawStaminaBar();
    void DrawBag();
    void DrawAim();

    CMyPlayer* owner = nullptr;   // 비소유
};
