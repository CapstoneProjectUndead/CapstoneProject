#pragma once
#include "Component.h"
#include "AnimationController.h"
#include "AnimationManager.h"
struct AnimationData;
struct CMeshComponent;
struct CMaterialComponent;
struct RenderUnit;

struct AnimLayer {
	std::string current_clip;
	float weight{ 1.0f };
	// animationManager.bone_masks의 인덱스와 일치시키기(-1이면 전체 적용)
	int mask_id{ -1 };
	float start_time{};
};

// 캐릭터별 애니메이션 셋
struct CharacterAnimSet {
	std::string idle;
	std::string walk;
	std::string run;
	std::string action; // 상반신용 기본 액션
};

// 소켓 렌더링에 필요한 리소스 정보 캐싱
struct SocketRenderCache {
	std::vector<RenderUnit> render_units{};
	int last_item_id{ -1 }; // 아이템이 바뀌었는지 체크용
	std::string model_name{}; // 아이템 아닐 때 모델이 바뀌었는지 체크
	EShaderName shader_name{ EShaderName::Inst };
};

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent();
	// owner 사용하기 때문에 owner Set 후 호출
	void Init(const CharacterAnimSet& animSet);
	void AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run);
	void PlayerSetState(const std::string& idle, const std::string& walk, const std::string& run);

	// layer 1
	void PlayAction(const std::string& clipName);
	// 렌더링 시에 호출
	AnimationData GetAnimationData();

	void Update(float deltaTime) override;
	void UpdateLayerWeights(float deltaTime);

	// Socket
	enum SOCKET_TYPE : unsigned int {
		HEAD,
		HAND_R,
		HAND_ROD_R,	// 다우징로드 소켓
		HAND_ROD_L,
		COUNT
	};
	struct Socket {
		int bone_index{-1};			// 연결될 본의 인덱스
		XMMATRIX local_offset{XMMatrixIdentity()};	// bone 위치에서의 offset(붙어있게 하려면 0으로)
	};
	XMVECTOR GetHeadPosition();	// 카메라 오프셋(=local_offset)이 있기 때문에 따로 연산 수행. base 애니메이션만 적용
	XMMATRIX GetSocketMatrix(SOCKET_TYPE type);
	// 현재 씬의 factory의 prototypes에 존재하는 mesh/material을 캐싱. itemID > 0이면 아이템 정보임
	void RenderSocketModel(SOCKET_TYPE type, int itemID, const std::string& modelName = "");

	std::string GetAttackClipByItem(int itemId) {
		switch (itemId) {
		case 14:
		case 19:
			return "Attack_swing";
		case 15:   return "Attack_hammer";
			//case : return "Attack_spray";
		case 17: return "Attack_wand";
		default: return "";
		}
	}
private:
	std::string GetDigClipByItem(int itemId) {
		if (itemId > 0 && itemId <= 4) return "Dig_shovel";
		else if (itemId > 4 && itemId <= 12) return "Dig_pick_ax";
		return "Dig_hand";
	}

	float current_time{};
	std::vector<AnimLayer> layers;
	CAnimationController controller;
	CharacterAnimSet anim_set;
	std::vector<Socket> sockets;
	std::unordered_map<SOCKET_TYPE, SocketRenderCache> render_cache;
	// pitch를 위한 클립(플레이어만 사용)
	AnimationClip up_clip;
	AnimationClip down_clip;
};

