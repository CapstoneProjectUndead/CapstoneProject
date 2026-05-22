#pragma once
#include "Player.h"
#include "MapGenerator/MapGenerator.h"

struct ClientFrameHistory 
{
    uint64       seq_num;
    float        duration;
    InputData    input{ false, false, false, false, false, false };
    XMFLOAT3     predicted_pos; // 내가 예측했던 결과 좌표
    XMFLOAT3     predicted_velocity;
    PLAYER_STATE state;
};

class CUser;
class CInventory;
class CQuickSlot;

class CMyPlayer :
    public CPlayer
{
public:
    CMyPlayer();
    ~CMyPlayer() {};

    virtual void Update(float elapsedTime) override;
    inline void PreUpdate(float elapsedTime);
    virtual void OnCollect(std::vector<std::unique_ptr<IRenderer>>& renderers) override;

public:
    std::weak_ptr<Session>   GetSessionWeak() const { return session; }
    std::shared_ptr<Session> GetSession() const { return session.lock(); }
    void SetSession(std::shared_ptr<Session> _session) { session = _session; }

    std::weak_ptr<CUser>      GetUserWeak() const { return user; }
    std::shared_ptr<CUser>    GetUser() const { return user.lock(); }
    void SetUser(std::shared_ptr<CUser> _user) { user = _user; }

    XMFLOAT3 GetServerVelocity() const { return server_velocity; }
    void SetServerVelocity(const XMFLOAT3 vel) { server_velocity = vel; }

    // 클라이언트 예측을 서버 기준에 맞게 다시 보정하는 코드
    void ReconcileFromServer(uint64_t last_seq, XMFLOAT3 serverPos);
    void SimulateMove(const InputData& input, float elapsedTime);

    void BeginSendInputPacket(float elapsedTime);

    bool GetIsReady() const { return is_ready; }
    void SetIsReady(bool ready) { is_ready = ready; }

    std::shared_ptr<CInventory> GetInventory()  const { return inventory; }
    void                        SetInventory(std::shared_ptr<CInventory> inven) { inventory = inven; }

    std::shared_ptr<CQuickSlot> GetQuickSlot()  const { return quick_slot; }
    void                        SetQuickSlot(std::shared_ptr<CQuickSlot> qs)  { quick_slot = qs; }

    uint64 GetGold() const { return gold; }
    void   SetGold(uint64 amount) { gold = amount; }
    void   AddGold(uint64 amount) { gold += amount; }

    void SetStaminaFromServer(uint32 stamina);
    void AddStamina(uint32 amount);

    bool GetDigAnimFinished() const   { return dig_anim_finished; }
    void SetDigAnimFinished(bool val) { dig_anim_finished = val; }

    float GetCHoldProgress() const { return min(c_hold_timer / 5.0f, 1.0f); }
    void  UpdateCHoldTimer(float delta) { c_hold_timer = min(c_hold_timer + delta, 5.0f); }
    void  ResetCHoldTimer()             { c_hold_timer = 0.0f; }

    // 넉백 + 스턴 (기본값 1.3f) 스턴은 원치 않으면 인자를 0으로 쓰면 된다.
    void ApplyKnockback(XMFLOAT3 dir, float force, float stun_duration = 1.3f);
    void ApplyStun(float time);
    void ApplyPossession();

    void ResetAll();

private:
    void ProcessRotation();
    void ProcessInput();
    void UpdateStamina(float elapsedTime);

    // 내 캐릭터 보간 코드 (클라이언트 예측이동이 없어서 추가된 함수)
    void InterpolateMyPlayer(float elapsedTime);

    // 서버 권위 방식 + 클라 예측 이동 방식
    void ServerAuthorityMove(const float elapsedTime);
    void CaptureInput(InputData& currentInput);
    void PredictMove(const InputData& input, float dt);
    void RecordClientFrameHistory(const ClientFrameHistory& history);

    void SendInputPacket(C_Input& inputPkt, const InputData& input);
    void SendPingToServer(const float elapsedTime);

    // 아이템 줍기
    void UseItem();

    // 빙의 관련
    void     UpdatePossession(float elapsedTime);
    XMFLOAT3 GetRandomPossessedTarget();

private:
    std::weak_ptr<Session> session;
    std::weak_ptr<CUser>   user;

    const float move_packet_send_delay = 1.0f / 60.0f;
    float move_packet_send_timer = move_packet_send_delay;

    // 프레임 시간을 누적시키기 위해서 추가한 변수
    float dt_accumulator = 0.0f;
    float dt_ping_accumulator = 0.0f;

    // 클라 예측 이동을 위한 시퀀스 넘버
    uint64                            client_seq_counter = 0;
    std::deque<ClientFrameHistory>    client_history_deq; // 시퀀스 장부
    InputData                         current_input;

    XMFLOAT3                          server_velocity{};

    std::shared_ptr<CInventory>       inventory;
    std::shared_ptr<CQuickSlot>       quick_slot;
    uint64                            gold;      // 소지금

    bool                              is_ready;
    bool                              dig_anim_finished{ false };

    float grounded_timer{ 0.1 };
    float accumulate_stamina{ 100.0f };
    bool  stamina_exhausted{ false };
    bool  start_jump{ false };

    XMFLOAT3 knockback_vel{};
    float    knockback_timer{ 0.0f };
    float    c_hold_timer{ 0.0f };

    std::vector<MapGenerator::Cell> possessed_nav_path;
    float                           possessed_path_refresh_timer = 0.0f;
    XMFLOAT3                        possessed_wander_target      = {};
    bool                            possessed_is_waiting         = false;
    float                           possessed_wait_timer         = 0.0f;
};

