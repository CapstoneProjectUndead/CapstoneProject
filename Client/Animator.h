#pragma once
#include "Component.h"
#include "AnimationController.h"
struct AnimationData;

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

/*
 hand index를 얻은 다음 GetSocketWorldMatrix로 GetHeadPosition처럼 matrix를 리턴
 local offset * matrix로 월드 변환->rendering 부분만

 void CInventory::AddItem(std::shared_ptr<CItem> item)로 얻은 아이템을 Rendering
 컴포넌트 2개를 citem이 들고 있고 render 호출 시 addinstance 호출
*/

class CAnimatorComponent : public CComponent
{
public:
	CAnimatorComponent();
	// owner 사용하기 때문에 owner Set 후 호출
	void Init(const CharacterAnimSet& animSet);
	void AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run);

	// layer 1
	void PlayAction(const std::string& clipName);
	AnimationData GetAnimationData();

	void Update(float deltaTime) override;
	void UpdateLayerWeights(float deltaTime);

	// Socket
	enum SOCKET_TYPE : unsigned int {
		HEAD,
		HAND_R,
		COUNT
	};
	struct Socket {
		int bone_index{-1};			// 연결될 본의 인덱스
		XMFLOAT4X4 local_offset;	// bone 위치에서의 offset(붙어있게 하려면 0으로)
	};
	XMVECTOR GetHeadPosition();	// 카메라 오프셋(=local_offset)이 있기 때문에 따로 연산 수행
	XMMATRIX GetSocketMatrix(SOCKET_TYPE type);
private:
	float current_time{};
	std::vector<AnimLayer> layers;
	CAnimationController controller;
	CharacterAnimSet anim_set;
	std::vector<Socket> sockets;
};