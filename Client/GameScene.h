#pragma once
#include "Scene.h"
#include "MapGenerator/MapGenerator.h"

class CWorldItem;
class CItem;
class CMineableObject;
enum class MINEABLEOBJECT_TYPE;

class CGameScene : public CScene
{
public:
    CGameScene();
    ~CGameScene();

    virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*) override;
    virtual void Initialize() override;
    virtual void Update(float elapsedTime) override;

    virtual void Enter() override;
    virtual void Exit() override;

    virtual void DrawUI() override;
    virtual bool IsUIInputEnabled() override;
    virtual bool IsMenuOpen() const override { return menu_open; }
    void SetButtonEvents();
public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt);
    void Handle_S_MapEnd(std::shared_ptr<Session> session, const S_MapEnd& pkt);

    virtual void Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt) override;
    virtual void Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt) override;
    virtual void Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt) override;
    virtual void Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt) override;
    virtual void Handle_S_RemoveItem(std::shared_ptr<Session> session, const S_RemoveItem& pkt) override;
    virtual void Handle_S_EquipItem(std::shared_ptr<Session>& session, const S_EquipItem& pkt) override;
    virtual void Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt) override;
    virtual void Handle_S_MineableList(std::shared_ptr<Session>& session, S_MineableList& pkt) override;
    virtual void Handle_S_DestroyMineable(std::shared_ptr<Session>& session, const S_DestroyMineable& pkt) override;
    virtual void Handle_S_UpdateDurability(std::shared_ptr<Session>& session, const S_UpdateDurability& pkt) override;
    virtual void Handle_S_PlaySound(std::shared_ptr<Session> session, S_PlaySound& pkt) override;
    void Handle_S_CHoldFail(std::shared_ptr<Session> session, S_CHoldFail& pkt);
    void Handle_S_ReturnZoneActive(std::shared_ptr<Session> session, const S_ReturnZoneActive& pkt);
    void Handle_S_PlayerReturned(std::shared_ptr<Session> session, const S_PlayerReturned& pkt);
    void Handle_S_GameSettlement(std::shared_ptr<Session> session, S_GameSettlement& pkt);

    // 싱글용 (Ghost 드롭 등 외부에서 호출 가능)
    void SpawnWorldItem(uint16 itemID, XMFLOAT3 position, int16 dur = -1);

    const std::vector<MonsterSpawnInfo>& GetMonsterSpawnInfo() const { return monster_spawn_info; }

    // 서버 권위 라운드 타이머 적용 (S_PlayerMove에서 받은 값)
    void  SetRoundTimer(float t) { round_timer = t; round_active = true; }
    float GetRoundTimer() const { return round_timer; }

    // 복귀존 (S_ReturnZoneActive에서 받은 값)
    bool            IsReturnActive()  const { return return_active; }
    const XMFLOAT3& GetReturnCenter() const { return return_center; }
    float           GetReturnRange()  const { return return_range; }

private:
    // 멀티용 (itemID는 도감번호, itemWorldId는 ObjectID)
    void SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position, int16 dur = -1);

    // 모델 원점이 제각각이라 로컬 AABB 바닥이 groundY에 닿도록 Y를 보정 (땅에 박힘 방지)
    void PlaceWorldItemOnGround(const std::shared_ptr<CWorldItem>& worldItem, float groundY);

    // 몬스터 respawn 관리
    void UpdateMonsters(float elapsedTime);

    // 플레이어 아이템 줍기
    void ProcessPickup();

    // 채굴 상호작용
    void ProcessVisibleObjectMining(float elapsedTime);
    void ProcessUnVisibleObjectMining(float elapsedTime);
    void FindNearestMineTarget(MINEABLEOBJECT_TYPE type);

    // 채굴 오브젝트 파괴 시 다우징로드 추적 목록 갱신 (싱글 전용)
    void RemoveTreasureFromDowsing(const XMFLOAT3& pos);

    // 공격
    void ProcessAttack(float elapsedTime);
    void ProcessMeleeAttack(float elapsedTime);
    void ProcessRangedAttack(float elapsedTime);
    void SprayAttack(float elapsedTime);

    void DropItemAtPlayerFeet(std::shared_ptr<CItem> item);

    // 빙의 해제 & 빈사 소생 (멀티 전용, 같은 C-홀드 메커니즘 공유)
    void UpdateCHoldAction(float elapsedTime);
    void DrawDePossessProgressBar();
    void DrawRescueProgressBar();
    void DrawCHoldPrompt();

    // 복귀존 월드 마커 (수평 원형 링, 카메라 view/proj로 투영)
    void DrawReturnMarker();

    // 빈사 3인칭 / 사망 관전 카메라가 벽을 뚫거나 맵 밖을 비추지 않도록,
    // 타깃과 카메라 사이에 벽이 있으면 카메라를 앞으로 당겨오는 충돌 보정.
    void ApplyCameraCollision();

    // 복귀 토스트 (본인 "복귀 완료" + 타 플레이어 "{id} 플레이어 복귀")
    void DrawReturnToasts();

    // 싱글 전용: my_player 위치를 매 틱 체크해서 복귀존 진입 감지 (멀티는 서버가 권위)
    void DetectMyPlayerReturn();

    // 사운드 관련
    void PlayMeleeAttackSound();
    void PlayBareHandDigSound(bool isBareHand, bool isMoving);

    // 정산 모달
    void DrawSettlementModal();
    void TriggerSinglePlayerSettlement();

    // 구조 포기 (빈사 본인 화면에 표시되는 자진 사망 버튼, 멀티 전용)
    void DrawGiveUpButton();
    void SendGiveUpRescue();

    // 상대 플레이어 상태 UI (오른쪽 상단, 최대 3명): 원형 사진 + HP바
    void DrawOpponentStatus();

    // ESC 메뉴 (ImGui). "타이틀로" 버튼만. 싱글은 일시정지, 멀티는 오버레이만.
    void DrawMenu();

    void SpawnTreasure(XMFLOAT3& pos);

private:
    std::vector<MapGenerator::InstanceData>  instance_data;
    std::vector<TreasureInfo>                treasures;
    std::vector<MonsterSpawnInfo>            monster_spawn_info;

    static constexpr float  PICKUP_RANGE       = 2.0f;
    static constexpr float  MINING_RANGE       = 0.45f;
    static constexpr float  BARE_HAND_MINING_RANGE = 0.3f;
    static constexpr uint32 WORLD_ITEM_ID_BASE = 50000; // 플레이어/몬스터 ID 범위와 겹치지 않는 값
    uint32                  world_item_id_counter = WORLD_ITEM_ID_BASE;

    // 라운드 타이머
    static constexpr float  ROUND_DURATION = 600.f; // 10분
    static constexpr float  RETURN_RANGE   = 1.0f; // 복귀존/맨홀 반경 (시각=판정)
    float                   round_timer    = 0.f;
    bool                    round_active   = false;

    // ESC 메뉴 열림 상태
    bool                    menu_open = false;

    // 복귀존 (서버 패킷으로 활성화)
    bool      return_active        = false;
    float     return_notify_timer  = 0.f;   // 활성화 알림 표시 타이머 (3초)
    XMFLOAT3  return_center {};
    float     return_range  = 0.f;

    // 복귀 토스트 (멀티: S_PlayerReturned로 트리거 / 싱글: DetectMyPlayerReturn으로 트리거)
    struct ReturnToast
    {
        std::string text;
        float timer;
        bool is_self;
    };
    std::vector<ReturnToast> return_toasts;
    static constexpr float   RETURN_TOAST_DURATION = 2.5f;

    bool              was_digging           = false;
    bool              bare_hand_dig_loop_playing      = true;
    CMineableObject*  mining_target         = nullptr;
    float             dig_sound_timer       = -1.0f;
    float             bare_hand_dig_timer   = 0.0f;

private:
    float             melee_attack_timer    = -1.0f;
    float             melee_attack_cooldown = -1.0f;
    float             spray_attack_timer    = -1.0f;
    float             spray_attack_cooldown = -1.0f;

    // 정산 결과 (S_GameSettlement 수신 또는 싱글 자체 계산)
    struct SettlementEntry
    {
        uint16      item_id;
        uint32      price;
        uint16      count;
        std::string name;
        std::string icon_path;
    };

    struct SettlementResult
    {
        std::vector<SettlementEntry> entries;
        uint32 base_coin          = 0;
        uint32 final_coin         = 0;
        bool   is_returned           = false;
        bool   all_returned_bonus = false;
    };

    SettlementResult  settlement_result;
    bool              show_settlement_modal = false;
};

