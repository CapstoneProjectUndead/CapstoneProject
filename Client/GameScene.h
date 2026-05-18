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
    void SetButtonEvents();
public:
    // 서버 패킷 처리 관련 함수들
    void Handle_S_MapData(std::shared_ptr<Session> session, const S_MapData& pkt);
    void Handle_S_MapEnd(std::shared_ptr<Session> session, const S_MapEnd& pkt);

    virtual void Handle_S_SpawnItem(std::shared_ptr<Session> session, const S_SpawnItem& pkt) override;
    virtual void Handle_S_SpawnItemList(std::shared_ptr<Session> session, S_Spawn_Item_List& pkt) override;
    virtual void Handle_S_DeSpawnItem(std::shared_ptr<Session> session, const S_DeSpawnItem& pkt) override;
    virtual void Handle_S_AddItem(std::shared_ptr<Session> session, const S_AddItem& pkt) override;
    virtual void Handle_S_AddItemList(std::shared_ptr<Session> session, S_AddItemList& pkt) override;
    virtual void Handle_S_RemoveItem(std::shared_ptr<Session> session, const S_RemoveItem& pkt) override;
    virtual void Handle_S_EquipItem(std::shared_ptr<Session>& session, const S_EquipItem& pkt) override;
    virtual void Handle_S_UseItem(std::shared_ptr<Session>& session, const S_UseItem& pkt) override;
    virtual void Handle_S_MineableList(std::shared_ptr<Session>& session, S_MineableList& pkt) override;
    virtual void Handle_S_DestroyMineable(std::shared_ptr<Session>& session, const S_DestroyMineable& pkt) override;
    virtual void Handle_S_UpdateDurability(std::shared_ptr<Session>& session, const S_UpdateDurability& pkt) override;
    virtual void Handle_S_PlaySound(std::shared_ptr<Session> session, S_PlaySound& pkt) override;
    void Handle_S_PossessionReleaseFail(std::shared_ptr<Session> session, S_PossessionReleaseFail& pkt);
    void Handle_S_ReturnZoneActive(std::shared_ptr<Session> session, const S_ReturnZoneActive& pkt);
    void Handle_S_PlayerReturned(std::shared_ptr<Session> session, const S_PlayerReturned& pkt);

    // 싱글용 (Ghost 드롭 등 외부에서 호출 가능)
    void SpawnWorldItem(uint16 itemID, XMFLOAT3 position);

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
    void SpawnWorldItem(uint16 itemID, uint32 itemWorldId, XMFLOAT3 position);

    // 몬스터 respawn 관리
    void UpdateMonsters(float elapsedTime);

    // 플레이어 아이템 줍기
    void ProcessPickup();

    // 채굴 상호작용
    void ProcessVisibleObjectMining(float elapsedTime);
    void ProcessUnVisibleObjectMining(float elapsedTime);
    void FindNearestMineTarget(MINEABLEOBJECT_TYPE type);

    // 공격
    void ProcessAttack(float elapsedTime);
    void ProcessMeleeAttack(float elapsedTime);
    void ProcessRangedAttack(float elapsedTime);
    void SprayAttack(float elapsedTime);

    void DropItemAtPlayerFeet(std::shared_ptr<CItem> item);

    // 빙의 해제 (멀티 전용)
    void ReleasePossession(float elapsedTime);
    void DrawDePossessProgressBar();

    // 복귀존 월드 마커 (수평 원형 링, 카메라 view/proj로 투영)
    void DrawReturnMarker();

    // 복귀 토스트 (본인 "복귀 완료" + 타 플레이어 "{id} 플레이어 복귀")
    void DrawReturnToasts();

    // 싱글 전용: my_player 위치를 매 틱 체크해서 복귀존 진입 감지 (멀티는 서버가 권위)
    void DetectMyPlayerReturn();

    // 사운드 관련
    void PlayMeleeAttackSound();

private:
    std::vector<MapGenerator::InstanceData>  instance_data;
    std::vector<TreasureInfo>                treasures;
    std::vector<MonsterSpawnInfo>            monster_spawn_info;

    static constexpr float  PICKUP_RANGE       = 2.0f;
    static constexpr float  MINING_RANGE       = 0.45f;
    static constexpr float  BARE_HAND_MINING_RANGE = 0.25f;
    static constexpr uint32 WORLD_ITEM_ID_BASE = 50000; // 플레이어/몬스터 ID 범위와 겹치지 않는 값
    uint32                  world_item_id_counter = WORLD_ITEM_ID_BASE;

    // 라운드 타이머
    static constexpr float  ROUND_DURATION = 300.f; // 5분
    float                   round_timer    = 0.f;
    bool                    round_active   = false;

    // 복귀존 (서버 패킷으로 활성화)
    bool      return_active = false;
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
    CMineableObject*  mining_target         = nullptr;
    float             dig_sound_timer       = -1.0f;
    float             bare_hand_dig_timer   = 0.0f;

private:
    float             melee_attack_timer    = -1.0f;
    float             melee_attack_cooldown = -1.0f;
    float             spray_attack_timer    = -1.0f;
    float             spray_attack_cooldown = -1.0f;
};

