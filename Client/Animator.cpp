#include "stdafx.h"
#include "Animator.h"
#include "Player.h"
#include "MyPlayer.h"
#include "Monster.h"
#include "GPUBufferStruct.h"

#include "SceneManager.h"
#include "ObjectFactory.h"
#include "MeshRenderer.h"

#include "ItemFactory.h"

CAnimatorComponent::CAnimatorComponent()
{
	layers.resize(2);
	layers[0].mask_id = -1; // 베이스는 전체
	layers[1].mask_id = 0;	// 상반신 mask
}

void CAnimatorComponent::Init(const CharacterAnimSet& animSet)
{
	// animation state 등록
	anim_set = animSet;

	// 상태 등록
	std::string idle{ "IdleState" }, walk{ "WalkState" }, run{ "RunState" };
	State walkState{ walk, animSet.walk };

	// 전이 규칙
	AddLocomotionTransitions(idle, walk, run);

	// socket 등록
	OBJECT_TYPE objType = owner->GetObjectType();
	uint8_t subType = 0;

	switch (objType) {
	case OBJECT_TYPE::PLAYER:
	{
		up_clip = CAnimationManager::GetInstance().GetClip("Ganga_up");
		down_clip = CAnimationManager::GetInstance().GetClip("Ganga_down");

		// 발 속도 맞추기
		walkState.play_speed = 2.5f;

		controller.AddState({ idle, animSet.idle });

		sockets.resize(SOCKET_TYPE::COUNT);
		int headIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "head");
		Socket headSocket{ headIdx, XMMatrixIdentity() };
		sockets[HEAD] = headSocket;

		int rHandIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "handR");
		// 손에 직선으로 위치
		XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(XMConvertToRadians(-90.0), XMConvertToRadians(0.0f), XMConvertToRadians(90.0f));
		Socket rHandSocket{ rHandIdx,rotMat };
		sockets[HAND_R] = rHandSocket;

		// 다우징 로드 소켓
		{
			constexpr float scaleValue{ 0.005757076 };
			XMMATRIX scaleMat{ XMMatrixScaling(scaleValue, scaleValue, scaleValue) };
			XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(XMConvertToRadians(90.0), XMConvertToRadians(0.0f), XMConvertToRadians(180.0f));
			XMMATRIX finalMat = scaleMat * rotMat;
			int lHandIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "handL");
			// 손에 직선으로 위치
			Socket lHandSocket{ lHandIdx, finalMat };
			sockets[HAND_ROD_L] = lHandSocket;

			int rHandIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "handR");
			// 손에 직선으로 위치
			Socket rHandSocket{ rHandIdx, finalMat };
			sockets[HAND_ROD_R] = rHandSocket;
		}
		PlayerSetState(idle, walk, run);
	}
	break;
	case OBJECT_TYPE::MONSTER:
	{
		sockets.resize(SOCKET_TYPE::HAND_R + 1);
		uint8_t monType = static_cast<uint8_t>(static_cast<CMonster*>(owner)->GetMonsterType());
		int idx = CAnimationManager::GetInstance().GetBoneIndex(objType, monType, "handR");
		XMMATRIX rotation = XMMatrixRotationY(XMConvertToRadians(90.0f));
		Socket socket{ idx, rotation };
		sockets[HAND_R] = socket;

		// Attack 상태 등록 및 전이 추가
		if (!animSet.action.empty()) {
			std::string attack{ "AttackState" };
			float attackSpeed = (static_cast<CMonster*>(owner)->GetMonsterType() == MON_TYPE::HUMAN_MONSTER) ? 2.0f : 1.0f;
			controller.AddState({ attack, animSet.action, attackSpeed });

			// Run → Attack
			Transition r2a;
			r2a.to_state = attack;
			r2a.duration = 0.1f;
			r2a.condition = [this]() {
				return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_ATTACK;
				};
			controller.AddTransition(run, r2a);

			// Idle → Attack (엣지 케이스)
			Transition i2a;
			i2a.to_state = attack;
			i2a.duration = 0.1f;
			i2a.condition = [this]() {
				return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_ATTACK;
				};
			controller.AddTransition(idle, i2a);

			// Attack → Run
			Transition a2r;
			a2r.to_state = run;
			a2r.duration = 0.2f;
			a2r.condition = [this]() {
				return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_TRACE;
				};
			controller.AddTransition(attack, a2r);

			// Attack → Idle
			Transition a2i;
			a2i.to_state = idle;
			a2i.duration = 0.2f;
			a2i.condition = [this]() {
				return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_IDLE;
				};
			controller.AddTransition(attack, a2i);
		}
	}
	break;
	}

	controller.AddState({ idle, animSet.idle });
	controller.AddState(walkState);
	controller.AddState({ run, animSet.run });
}

void CAnimatorComponent::AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run)
{
	// Idle -> Walk
	Transition i2w;
	i2w.to_state = walk;
	i2w.duration = 0.2f;
	i2w.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::WALK;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			AI_STATE s = static_cast<CMonster*>(owner)->GetAIState();
			return s == AI_STATE::MONSTER_PATROL;
		}
		return false;
		};
	controller.AddTransition(idle, i2w);

	// Walk -> Idle
	Transition w2i;
	w2i.to_state = idle;
	w2i.duration = 0.2f;
	w2i.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::IDLE;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_IDLE;
		}
		return false;
		};
	controller.AddTransition(walk, w2i);

	// Walk -> Run
	Transition w2r;
	w2r.to_state = run;
	w2r.duration = 0.2f;
	w2r.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::RUN;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_TRACE;
		}
		return false;
		};
	controller.AddTransition(walk, w2r);

	// Run -> Walk
	Transition r2w;
	r2w.to_state = walk;
	r2w.duration = 0.2f;
	r2w.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::WALK;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_PATROL;
		}
		return false;
		};
	controller.AddTransition(run, r2w);

	// Idle -> Run
	Transition i2r;
	i2r.to_state = run;
	i2r.duration = 0.2f;
	i2r.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER)
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::RUN;
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER)
			return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_TRACE;
		return false;
		};
	controller.AddTransition(idle, i2r);

	// Run -> Idle
	Transition r2i;
	r2i.to_state = idle;
	r2i.duration = 0.2f;
	r2i.condition = [this]() {
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER)
			return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::IDLE;
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER)
			return static_cast<CMonster*>(owner)->GetAIState() == AI_STATE::MONSTER_IDLE;
		return false;
		};
	controller.AddTransition(run, r2i);
}

void CAnimatorComponent::PlayerSetState(const std::string& idle, const std::string& walk, const std::string& run)
{
	// dig state(action으로도 가능)
	const std::string dig{ "DigState" };
	controller.AddState({ dig, "Dig", 3, false });
	Transition i2d;
	i2d.to_state = dig;
	i2d.duration = 0.2f;
	i2d.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::DIG;
		};
	controller.AddTransition(idle, i2d);

	// Walk/Run → Dig (이동 직후 클릭 시 Walk/Run 블렌딩 끝난 후에도 Dig로 전이 가능)
	Transition w2d;
	w2d.to_state = dig;
	w2d.duration = 0.2f;
	w2d.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::DIG;
		};
	controller.AddTransition(walk, w2d);

	Transition d2i;
	d2i.to_state = idle;
	d2i.duration = 0.2f;
	d2i.condition = [this]() {
		// 애니메이션이 끝났거나
		// PlayerSetState 안의 d2i 조건 수정
		if (controller.GetCurrentState() == "DigState" && controller.GetPlayCount() >= 1.0f) {
			auto* player = static_cast<CMyPlayer*>(owner);
			player->SetDigAnimFinished(true);
			player->SetState(PLAYER_STATE::IDLE);
			return true;
		}
		// 취소 상태(예: 이동 입력)
		if (static_cast<CPlayer*>(owner)->GetState() != PLAYER_STATE::DIG) return true;
		return false;
		};
	controller.AddTransition(dig, d2i);

	// jump state
	const std::string jump{ "JumpState" };
	controller.AddState({ jump, "Jump" });

	Transition i2j;
	i2j.to_state = jump;
	i2j.duration = 0.2f;
	i2j.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::JUMP;
		};
	controller.AddTransition(idle, i2j);

	Transition w2j;
	w2j.to_state = jump;
	w2j.duration = 0.2f;
	w2j.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::JUMP;
		};
	controller.AddTransition(walk, w2j);

	Transition r2j;
	r2j.to_state = jump;
	r2j.duration = 0.2f;
	r2j.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() == PLAYER_STATE::JUMP;
		};
	controller.AddTransition(run, r2j);

	Transition j2i;
	j2i.to_state = idle;
	j2i.duration = 0.2f;
	j2i.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() != PLAYER_STATE::JUMP;
		};
	controller.AddTransition(jump, j2i);

	Transition j2w;
	j2w.to_state = walk;
	j2w.duration = 0.2f;
	j2w.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() != PLAYER_STATE::JUMP;
		};
	controller.AddTransition(jump, j2w);

	Transition j2r;
	j2r.to_state = run;
	j2r.duration = 0.2f;
	j2r.condition = [this]() {
		return static_cast<CPlayer*>(owner)->GetState() != PLAYER_STATE::JUMP;
		};
	controller.AddTransition(jump, j2r);
}

void CAnimatorComponent::PlayAction(const std::string& clipName)
{
	if (layers[1].current_clip == clipName) return;

	layers[1].current_clip = clipName;
	// 현재 흐르고 있는 전체 시간을 시작 시간으로
	layers[1].start_time = current_time;
	layers[1].weight = 0.0f;
}

XMVECTOR CAnimatorComponent::GetHeadPosition()
{
	int boneIndex = sockets[HEAD].bone_index;
	if (boneIndex == -1) return XMVECTOR{};

	// 현재 실제로 베이스 레이어에서 재생 중인 클립을 가져옴
	std::string activeClip = layers[0].current_clip;
	if (activeClip.empty()) return XMVECTOR{};

	float relativeTime = current_time - layers[0].start_time;
	XMMATRIX matrix = CAnimationManager::GetInstance().GetBoneSocketMatrix(activeClip, relativeTime, boneIndex);
	return matrix.r[3];
}

XMMATRIX CAnimatorComponent::GetSocketMatrix(SOCKET_TYPE type)
{
	XMMATRIX worldMatrix = XMLoadFloat4x4(&owner->world_matrix);
	if (static_cast<size_t>(type) >= sockets.size()) return worldMatrix;

	const auto& socket = sockets[type];
	if (socket.bone_index == -1) return worldMatrix;

	auto& manager = CAnimationManager::GetInstance();
	std::string clipA = layers[0].current_clip;
	float relTimeA = current_time - layers[0].start_time;

	// Dig일 경우 소켓 시간도 마지막에 고정
	if (clipA == "Dig" || clipA == "DigState") {
		auto& animA = manager.GetClip(clipA);
		float maxTime = (animA.total_frames > 0) ? (animA.total_frames - 1) / 60.0f : 0.0f;
		if (relTimeA > maxTime) relTimeA = maxTime;
	}

	XMMATRIX matA = manager.GetBoneSocketMatrix(clipA, relTimeA, socket.bone_index);
	XMMATRIX finalLocalMat = matA;

	if (controller.IsBlending()) {
		std::string clipB = controller.GetNextClip();
		if (clipB.empty()) clipB = controller.GetCurrentClip();

		if (!clipB.empty() && clipB != clipA) {
			float relTimeB = controller.GetCurrentClipTime();
			XMMATRIX matB = manager.GetBoneSocketMatrix(clipB, relTimeB, socket.bone_index);
			float weight = controller.GetWeight();

			// 유저님이 원하는 단순 행렬 선형 보간 (Matrix Lerp)
			finalLocalMat = matA * (1.0f - weight) + matB * weight;
		}
	}
	else if (!layers[1].current_clip.empty() && layers[1].weight > 0.0f) {
		float relTime1 = current_time - layers[1].start_time;
		XMMATRIX mat1 = manager.GetBoneSocketMatrix(layers[1].current_clip, relTime1, socket.bone_index);
		finalLocalMat = matA * (1.0f - layers[1].weight) + mat1 * layers[1].weight;
	}

	return socket.local_offset * finalLocalMat * worldMatrix;
}


void CAnimatorComponent::RenderSocketModel(SOCKET_TYPE type, int itemID, const std::string& modelName)
{
	if (itemID == -1) return;

	auto& cache = render_cache[type];

	// 아이템이 바뀌었을 때만 load
	if (cache.last_item_id != itemID && itemID > 0) {
		auto proto = CSceneManager::GetInstance().GetActiveScene()->GetFactory()->GetPrototype(ItemFactory::GetModelName(itemID));

		if (proto) {
			cache.mesh = proto->GetComponent<CMeshComponent>();
			cache.mat = proto->GetComponent<CMaterialComponent>();
			cache.last_item_id = itemID;
		}
	}
	else if (cache.last_item_id != itemID && NULL == itemID) {	// item이 아니면 modelName 사용
		auto proto = CSceneManager::GetInstance().GetActiveScene()->GetFactory()->GetPrototype(modelName);

		if (proto) {
			cache.mesh = proto->GetComponent<CMeshComponent>();
			cache.mat = proto->GetComponent<CMaterialComponent>();
			cache.last_item_id = itemID;
		}
	}

	// material은 없을 수 있음
	if (cache.mesh) {
		XMMATRIX socketMat = GetSocketMatrix(type);
		CSceneManager::GetInstance().GetRanderers()["inst"]->AddInstance(cache.mesh->GetMesh().get(), cache.mat, Matrix4x4::XMMatrixToFloat4x4(socketMat), false);
	}
}

void CAnimatorComponent::UpdateLayerWeights(float deltaTime)
{
	// Layer 0 (Base)는 항상 Weight 1.0이므로 관리 생략

	// Layer 1 (Action/UpperBody) 페이드 인/아웃
	if (!layers[1].current_clip.empty()) {
		layers[1].weight += deltaTime * 5.0f; // 0.2초 Fade-in
		if (layers[1].weight > 1.0f) layers[1].weight = 1.0f;
	}
	else {
		layers[1].weight -= deltaTime * 5.0f; // Fade-out
		if (layers[1].weight < 0.0f) layers[1].weight = 0.0f;
	}
}

AnimationData CAnimatorComponent::GetAnimationData()
{
	AnimationData data{};
	auto& manager = CAnimationManager::GetInstance();

	std::string baseClip = layers[0].current_clip;
	if (baseClip.empty()) return data;

	auto& animA = manager.GetClip(baseClip);
	data.start_offset_A = animA.start_matrix_offset;
	data.bone_count = animA.bone_count;

	// 현재 시간 기반 프레임 계산
	float relativeTimeA = current_time - layers[0].start_time;
	uint32_t frameA = (uint32_t)(relativeTimeA * 60.0f);
	uint32_t maxF = (animA.total_frames > 0) ? (uint32_t)animA.total_frames - 1 : 0;

	// [수정] Dig 계열만 마지막 프레임 고정, 나머지는 루프(%) 적용
	if (baseClip == "Dig" || baseClip == "DigState") {
		data.cur_frame_A = (frameA < maxF) ? frameA : maxF;
	}
	else {
		data.cur_frame_A = (animA.total_frames > 0) ? (frameA % animA.total_frames) : 0;
	}

	// Pitch 로직 (기존 유지)
	float pitchWeight{ -1.0f };
	bool isUp{};
	if (!up_clip.name.empty()) {
		float pitch = owner->pitch / 90.f;
		if (pitch <= 0.0f) isUp = true;
		pitchWeight = std::abs(pitch);
	}

	// 블렌딩 처리 (단순 선형 보간 유지)
	if (!layers[1].current_clip.empty() && layers[1].weight > 0.0f) {
		auto& animB = manager.GetClip(layers[1].current_clip);
		data.start_offset_B = animB.start_matrix_offset;
		float relativeTimeB = current_time - layers[1].start_time;
		data.cur_frame_B = (uint32_t)(relativeTimeB * 60.0f) % animB.total_frames;
		data.blend_weight = layers[1].weight;
		data.mask_id = layers[1].mask_id;
	}
	else if (controller.IsBlending()) {
		std::string clipB = controller.GetNextClip();
		if (clipB.empty()) clipB = controller.GetCurrentClip();

		if (!clipB.empty()) {
			auto& animB = manager.GetClip(clipB);
			data.start_offset_B = animB.start_matrix_offset;

			// 다음 클립(B)의 프레임 계산
			float clipTimeB = controller.GetCurrentClipTime();
			data.cur_frame_B = (uint32_t)(clipTimeB * 60.0f) % animB.total_frames;

			data.blend_weight = controller.GetWeight(); // 10:0 -> 0:10 리니어 블렌딩
			data.mask_id = -1;
		}
	}
	else if (pitchWeight > 0.0f) {
		if (isUp) data.start_offset_B = up_clip.start_matrix_offset;
		else data.start_offset_B = down_clip.start_matrix_offset;
		data.cur_frame_B = 0;
		data.blend_weight = pitchWeight;
		data.mask_id = 0;
	}
	else {
		data.blend_weight = 0.0f;
		data.mask_id = -1;
	}

	return data;
}


void CAnimatorComponent::Update(float deltaTime)
{
	if (owner == nullptr) return;

	current_time += deltaTime;
	controller.Update(deltaTime);
	UpdateLayerWeights(deltaTime);

	std::string nextClip = controller.GetCurrentClip();

	// [수정] 블렌딩 중일 때의 처리 로직
	if (controller.IsBlending()) {
		// 1. 단발성 애니메이션(Dig)이 블렌딩 중이면? -> 포즈를 고정해야 하므로 시간을 같이 밀어줌
		if (layers[0].current_clip == "Dig" || layers[0].current_clip == "DigState") {
			layers[0].start_time += deltaTime;
		}
		// 2. 루프 애니메이션(Walk, Idle 등)이 블렌딩 중이면? -> 가만히 둬야 current_time에 의해 계속 재생됨
		else {
			// 아무것도 안 함 (start_time 유지)
		}
	}
	else {
		// 블렌딩 중이 아닐 때는 실시간으로 상태 동기화
		float clipTime = controller.GetCurrentClipTime();
		layers[0].current_clip = nextClip;
		layers[0].start_time = current_time - clipTime;
	}
}