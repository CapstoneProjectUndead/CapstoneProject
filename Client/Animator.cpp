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
	State runState{ run, animSet.run };

	// 전이 규칙
	AddLocomotionTransitions(idle, walk, run);

	// owner 안전성 검사
	if (!owner) return;

	// socket 등록
	OBJECT_TYPE objType = owner->GetObjectType();
	uint8_t subType = 0;

	switch (objType) {
	case OBJECT_TYPE::PLAYER:
	{
		up_clip = CAnimationManager::GetInstance().GetClip("Ganga_up");
		down_clip = CAnimationManager::GetInstance().GetClip("Ganga_down");

		// 발 속도 맞추기
		walkState.play_speed = 2.2f;
		runState.play_speed = 2.2f;

		controller.AddState({ idle, animSet.idle });

		sockets.resize(SOCKET_TYPE::COUNT);
		int headIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "head");
		Socket headSocket{ headIdx, XMMatrixIdentity() };
		sockets[HEAD] = headSocket;

		int rHandIdx = CAnimationManager::GetInstance().GetBoneIndex(objType, NULL, "handR");
		// 손에 직선으로 위치
		XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0), XMConvertToRadians(70.0f), XMConvertToRadians(-70.0f));
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

		auto* monsterOwner = dynamic_cast<CMonster*>(owner);
		if (!monsterOwner) return;

		uint8_t monType = static_cast<uint8_t>(monsterOwner->GetMonsterType());
		int idx = CAnimationManager::GetInstance().GetBoneIndex(objType, monType, "handR");
		// offset matrix
		XMMATRIX mScale = XMMatrixScalingFromVector(XMVectorSet(0.2635728f, 0.2635728f, 0.2635728f, 0.0f));
		XMMATRIX mRot = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(90.0f), XMConvertToRadians(-90.0f));
		XMMATRIX mTrans = XMMatrixTranslationFromVector(XMVectorSet(-0.003635712f, 0.02177059f, 0.01650229f, 0.0f));
		Socket socket{ idx, mScale * mRot * mTrans };
		sockets[HAND_R] = socket;

		// Attack 상태 등록 및 전이 추가
		if (!animSet.action.empty()) {
			std::string attack{ "AttackState" };
			float attackSpeed = (monsterOwner->GetMonsterType() == MON_TYPE::HUMAN_MONSTER) ? 2.0f : 1.0f;
			controller.AddState({ attack, animSet.action, attackSpeed });

			// Run → Attack
			Transition r2a;
			r2a.to_state = attack;
			r2a.duration = 0.1f;
			r2a.condition = [this]() {
				auto* monster = dynamic_cast<CMonster*>(owner);
				if (!monster) return false;
				return monster->GetAIState() == AI_STATE::MONSTER_ATTACK;
				};
			controller.AddTransition(run, r2a);

			// Idle → Attack (엣지 케이스)
			Transition i2a;
			i2a.to_state = attack;
			i2a.duration = 0.1f;
			i2a.condition = [this]() {
				auto* monster = dynamic_cast<CMonster*>(owner);
				if (!monster) return false;
				return monster->GetAIState() == AI_STATE::MONSTER_ATTACK;
				};
			controller.AddTransition(idle, i2a);

			// Attack → Run
			Transition a2r;
			a2r.to_state = run;
			a2r.duration = 0.2f;
			a2r.condition = [this]() {
				auto* monster = dynamic_cast<CMonster*>(owner);
				if (!monster) return false;
				auto s = monster->GetAIState();
				return s == AI_STATE::MONSTER_TRACE || s == AI_STATE::MONSTER_FLEE;
				};
			controller.AddTransition(attack, a2r);

			// Attack → Idle
			Transition a2i;
			a2i.to_state = idle;
			a2i.duration = 0.2f;
			a2i.condition = [this]() {
				auto* monster = dynamic_cast<CMonster*>(owner);
				if (!monster) return false;
				return monster->GetAIState() == AI_STATE::MONSTER_IDLE;
				};
			controller.AddTransition(attack, a2i);
		}
	}
	break;
	}

	controller.AddState({ idle, animSet.idle });
	controller.AddState(walkState);
	controller.AddState(runState);
}

void CAnimatorComponent::AddLocomotionTransitions(const std::string& idle, const std::string& walk, const std::string& run)
{
	// Idle -> Walk
	Transition i2w;
	i2w.to_state = walk;
	i2w.duration = 0.2f;
	i2w.condition = [this]() {
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::WALK) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			if (!monster) return false;
			AI_STATE s = monster->GetAIState();
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
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::IDLE) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			return monster ? (monster->GetAIState() == AI_STATE::MONSTER_IDLE) : false;
		}
		return false;
		};
	controller.AddTransition(walk, w2i);

	// Walk -> Run
	Transition w2r;
	w2r.to_state = run;
	w2r.duration = 0.2f;
	w2r.condition = [this]() {
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::RUN) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			if (!monster) return false;
			auto s = monster->GetAIState();
			return s == AI_STATE::MONSTER_TRACE || s == AI_STATE::MONSTER_FLEE;
		}
		return false;
		};
	controller.AddTransition(walk, w2r);

	// Run -> Walk
	Transition r2w;
	r2w.to_state = walk;
	r2w.duration = 0.2f;
	r2w.condition = [this]() {
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::WALK) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			return monster ? (monster->GetAIState() == AI_STATE::MONSTER_PATROL) : false;
		}
		return false;
		};
	controller.AddTransition(run, r2w);

	// Idle -> Run
	Transition i2r;
	i2r.to_state = run;
	i2r.duration = 0.2f;
	i2r.condition = [this]() {
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::RUN) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			if (!monster) return false;
			auto s = monster->GetAIState();
			return s == AI_STATE::MONSTER_TRACE || s == AI_STATE::MONSTER_FLEE;
		}
		return false;
		};
	controller.AddTransition(idle, i2r);

	// Run -> Idle
	Transition r2i;
	r2i.to_state = idle;
	r2i.duration = 0.2f;
	r2i.condition = [this]() {
		if (!owner) return false;
		if (owner->GetObjectType() == OBJECT_TYPE::PLAYER) {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? (player->GetState() == PLAYER_STATE::IDLE) : false;
		}
		else if (owner->GetObjectType() == OBJECT_TYPE::MONSTER) {
			auto* monster = dynamic_cast<CMonster*>(owner);
			return monster ? (monster->GetAIState() == AI_STATE::MONSTER_IDLE) : false;
		}
		return false;
		};
	controller.AddTransition(run, r2i);
}

void CAnimatorComponent::PlayerSetState(const std::string& idle, const std::string& walk, const std::string& run)
{
	// dig state(action으로도 가능)
	const std::string dig{ "DigState" };
	controller.AddState({ dig, "Dig_hand", 3.0f, false });

	Transition i2d;
	i2d.to_state = dig;
	i2d.duration = 0.2f;
	i2d.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		if (!player) return false;
		if (player->GetState() == PLAYER_STATE::DIG) {
			std::string targetClip = GetDigClipByItem(player->GetEquippedItemId());

			controller.ModifyStateClip("DigState", targetClip);
			return true;
		}
		return false;
		};
	controller.AddTransition(idle, i2d);

	// Walk/Run → Dig (이동 직후 클릭 시 Walk/Run 블렌딩 끝난 후에도 Dig로 전이 가능)
	Transition w2d;
	w2d.to_state = dig;
	w2d.duration = 0.2f;
	w2d.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		if (!player) return false;
		if (player->GetState() == PLAYER_STATE::DIG) {
			std::string targetClip = GetDigClipByItem(player->GetEquippedItemId());

			controller.ModifyStateClip("DigState", targetClip);
			return true;
		}
		return false;
		};
	controller.AddTransition(walk, w2d);

	Transition d2i;
	d2i.to_state = idle;
	d2i.duration = 0.2f;
	d2i.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		if (!player) return true;

		// dig 애니메이션이 한 번 끝났으면 idle로 전이.
		if (controller.GetCurrentState() == "DigState" && controller.GetPlayCount() >= 1) {
			if (auto* myPlayer = dynamic_cast<CMyPlayer*>(owner)) {
				myPlayer->SetDigAnimFinished(true);
				myPlayer->SetState(PLAYER_STATE::IDLE);
			}
			return true;
		}

		// 서버 상태가 더 이상 DIG가 아니면 종료
		if (player->GetState() != PLAYER_STATE::DIG) return true;
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
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() == PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(idle, i2j);

	Transition w2j;
	w2j.to_state = jump;
	w2j.duration = 0.2f;
	w2j.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() == PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(walk, w2j);

	Transition r2j;
	r2j.to_state = jump;
	r2j.duration = 0.2f;
	r2j.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() == PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(run, r2j);

	Transition j2i;
	j2i.to_state = idle;
	j2i.duration = 0.2f;
	j2i.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() != PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(jump, j2i);

	Transition j2w;
	j2w.to_state = walk;
	j2w.duration = 0.2f;
	j2w.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() != PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(jump, j2w);

	Transition j2r;
	j2r.to_state = run;
	j2r.duration = 0.2f;
	j2r.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() != PLAYER_STATE::JUMP) : false;
		};
	controller.AddTransition(jump, j2r);

	// possess
	const std::string possess{ "PossessState" };
	controller.AddState({ possess, "Ganga_run2" });

	// 모든 주요 상태(Idle, Walk, Run, Jump, Dig)에서 빙의로 가는 전이 추가
	std::vector<std::string> allStates = { idle, walk, run, "JumpState", "DigState" };
	for (const auto& from : allStates) {
		Transition any2p;
		any2p.to_state = possess;
		any2p.duration = 0.1f;
		any2p.condition = [this]() {
			auto* player = dynamic_cast<CPlayer*>(owner);
			if (!player) return false;
			if (player->GetIsPossessed()) {
				prev_action = layers[1].current_clip;
				PlayAction("");
				return true;
			}
			return false;
			};
		controller.AddTransition(from, any2p);
	}

	Transition p2i;
	p2i.to_state = idle;
	p2i.duration = 0.2f;
	p2i.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		if (!player) return false;
		if (!player->GetIsPossessed()) {
			PlayAction(prev_action);
			return true;
		}
		return false;
		};
	controller.AddTransition(possess, p2i);

	// 기절 상태 (STUNNED): is_stunned bool로 전이, 클립은 Ganga_tired
	// 빙의(possess)보다 우선 — 빙의된 플레이어가 맞으면 기절 애니메이션이 빙의를 덮어쓴다
	const std::string stunned{ "StunnedState" };
	controller.AddState({ stunned, "Ganga_tired", 1.0f, true });

	std::vector<std::string> activeStates = { idle, walk, run, "JumpState", "DigState", possess };
	for (const auto& from : activeStates) {
		Transition any2s;
		any2s.to_state = stunned;
		any2s.duration = 0.2f;
		any2s.condition = [this]() {
			auto* player = dynamic_cast<CPlayer*>(owner);
			return player ? player->GetIsStunned() : false;
			};
		controller.AddTransition(from, any2s);
	}

	// 스턴 풀린 직후 여전히 빙의 중이면 idle 경유 없이 바로 possess로 (블렌딩 2단 방지)
	Transition s2p;
	s2p.to_state = possess;
	s2p.duration = 0.2f;
	s2p.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		if (!player) return false;
		return !player->GetIsStunned() && player->GetIsPossessed();
		};
	controller.AddTransition(stunned, s2p);

	Transition s2i;
	s2i.to_state = idle;
	s2i.duration = 0.2f;
	s2i.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? !player->GetIsStunned() : false;
		};
	controller.AddTransition(stunned, s2i);

	// 빈사 상태 (ALMOST_DEAD): Collapse(쓰러짐 1회) → AlmostDeadState(엎드린 채 유지)
	const std::string collapse{ "CollapseState" };
	const std::string almostDead{ "AlmostDeadState" };

	controller.AddState({ collapse, "Collapse", 1.0f, false });
	controller.AddState({ almostDead, "Dead", 1.0f, true });

	std::vector<std::string> allStatesForAlmostDead = { idle, walk, run, "JumpState", "DigState", "PossessState", stunned };
	for (const auto& from : allStatesForAlmostDead) {
		Transition any2c;
		any2c.to_state = collapse;
		any2c.duration = 0.1f;
		any2c.condition = [this]() {
			auto* player = dynamic_cast<CPlayer*>(owner);
			if (!player) return false;
			if (player->GetState() == PLAYER_STATE::ALMOST_DEAD) {
				// 빈사 진입 시 상체 Action 레이어가 섞여서 기괴해지는 것을 방지
				PlayAction("");
				return true;
			}
			return false;
			};
		controller.AddTransition(from, any2c);
	}

	// Collapse 1회 재생 완료 시 AlmostDeadState(유지)로 자동 전이
	Transition c2ad;
	c2ad.to_state = almostDead;
	c2ad.duration = 0.2f;
	c2ad.condition = [this]() {
		if (controller.GetCurrentState() == "CollapseState" && controller.GetPlayCount() >= 1) {
			return true;
		}
		return false;
		};
	controller.AddTransition(collapse, c2ad);

	// 구조 복귀: 상태가 ALMOST_DEAD가 아니게 되면 idle로
	Transition ad2i;
	ad2i.to_state = idle;
	ad2i.duration = 0.2f;
	ad2i.condition = [this]() {
		auto* player = dynamic_cast<CPlayer*>(owner);
		return player ? (player->GetState() != PLAYER_STATE::ALMOST_DEAD) : false;
		};
	controller.AddTransition(almostDead, ad2i);
}

void CAnimatorComponent::OnChangeEquippedItem(int itemId)
{
	if (owner == nullptr || owner->GetObjectType() != OBJECT_TYPE::PLAYER)
		return;

	bool isShovel = (itemId == 1);

	if (isShovel) {
		// 유니티 X=0, Y=180, Z=70
		XMMATRIX baseRotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(0.0f), XMConvertToRadians(180.0f), XMConvertToRadians(70.0f));

		constexpr float tiltAngle = XMConvertToRadians(-90.0f);
		XMMATRIX matTilt = XMMatrixRotationX(tiltAngle);
		XMMATRIX finalRotation = matTilt * baseRotation;
		sockets[HAND_R].local_offset = finalRotation;

		controller.ModifyStateClip("IdleState", "shovel_idle");
	}
	else {
		sockets[HAND_R].local_offset = XMMatrixRotationRollPitchYaw(
			XMConvertToRadians(0.0f),
			XMConvertToRadians(70.0f),
			XMConvertToRadians(-70.0f)
		);

		controller.ModifyStateClip("IdleState", anim_set.idle);
	}

	if (controller.GetCurrentState() == "IdleState") {
		controller.ResetPlayCount();
	}
}

void CAnimatorComponent::PlayAction(const std::string& clipName, bool isLoop)
{
	if (layers[1].current_clip == clipName) return;

	layers[1].current_clip = clipName;
	layers[1].elapsed_time = 0.0f;
	layers[1].weight = 0.0f;
	layers[1].is_loop = isLoop;
}

XMVECTOR CAnimatorComponent::GetHeadPosition()
{
	int boneIndex = sockets[HEAD].bone_index;
	if (boneIndex == -1) return XMVECTOR{};

	// 현재 실제로 베이스 레이어에서 재생 중인 클립을 가져옴
	std::string activeClip = layers[0].current_clip;
	if (activeClip.empty()) return XMVECTOR{};

	float relativeTime = layers[0].elapsed_time;
	XMMATRIX matrix = CAnimationManager::GetInstance().GetBoneSocketMatrix(activeClip, relativeTime, boneIndex);
	return matrix.r[3];
}

XMMATRIX CAnimatorComponent::GetSocketMatrix(SOCKET_TYPE type)
{
	XMMATRIX worldMatrix = XMLoadFloat4x4(&owner->world_matrix);
	if (static_cast<size_t>(type) >= sockets.size()) return worldMatrix;

	// 인덱스가 유효한지 확인
	const auto& socket = sockets[type];
	if (socket.bone_index == -1) return worldMatrix;

	auto& manager = CAnimationManager::GetInstance();

	// 행렬 계산(base + action)
	// Layer 0 (Base)
	float relTime0 = layers[0].elapsed_time;
	XMMATRIX mat0 = manager.GetBoneSocketMatrix(layers[0].current_clip, relTime0, socket.bone_index);

	XMMATRIX finalLocalMat = mat0;

	// Layer 1 (Action)이 재생 중이라면 블렌딩
	if (!layers[1].current_clip.empty() && layers[1].weight > 0.0f) {
		float relTime1 = layers[1].elapsed_time;
		XMMATRIX mat1 = manager.GetBoneSocketMatrix(layers[1].current_clip, relTime1, socket.bone_index);
		finalLocalMat = mat0 * (1.0f - layers[1].weight) + mat1 * layers[1].weight;
	}
	
	return socket.local_offset * finalLocalMat * worldMatrix;
}

void CAnimatorComponent::RenderSocketModel(SOCKET_TYPE type, int itemID, const std::string& modelName)
{
	if (itemID < 0) {
		render_cache[type].last_item_id = -1;	// 초기화
		return;
	}

	auto& cache = render_cache[type];

	// 아이템이 바뀌었을 때만 load
	if (cache.last_item_id != itemID && itemID > 0) {
		auto proto = CSceneManager::GetInstance().GetFactory()->GetPrototype(ItemFactory::GetModelName(itemID));
		
		if (proto) {
			// item은 모두 inst rnederer 사용
			for (CMeshRendererComponent* renderer : proto->GetComponents<CMeshRendererComponent>()) {
				cache.render_units = renderer->GetRenderUnits();
			}
			cache.last_item_id = itemID;
			cache.model_name = "";
		}
	}
	else if (cache.last_item_id != itemID && NULL == itemID && modelName != cache.model_name) {	// item이 아니면 modelName 사용
		auto proto = CSceneManager::GetInstance().GetFactory()->GetPrototype(modelName);

		if (proto) {
			for (CMeshRendererComponent* renderer : proto->GetComponents<CMeshRendererComponent>()) {
				cache.render_units = renderer->GetRenderUnits();
				cache.shader_name = renderer->GetShader();
			}
			cache.last_item_id = itemID;
			cache.model_name = modelName;
		}
	}

	// material은 없을 수 있음
	for (const RenderUnit& unit : cache.render_units) {
		if (unit.mesh) {
			XMMATRIX socketMat = GetSocketMatrix(type);
			CSceneManager::GetInstance().GetRanderers()[cache.shader_name]->AddInstance(unit.mesh->GetMesh(), unit.material, Matrix4x4::XMMatrixToFloat4x4(socketMat), unit.submesh_index, false);
		}
	}
}

void CAnimatorComponent::UpdateLayerWeights(float deltaTime)
{
	// Layer 0 (Base)는 항상 Weight 1.0이므로 관리 생략

	// Layer 1 (Action/UpperBody) 페이드 인/아웃
	if (!layers[1].current_clip.empty()) {
		auto& anim = CAnimationManager::GetInstance().GetClip(layers[1].current_clip);
		float duration = (float)anim.total_frames / 60.0f;
		
		layers[1].elapsed_time += deltaTime;
		float elapsed = layers[1].elapsed_time;

		// 애니메이션 재생 중 (Fade-in)
		if (layers[1].is_loop) {
			// 페이드 인 상태 유지
			if (layers[1].weight < 1.0f) {
				layers[1].weight += deltaTime * 10.0f;
			}

			// 시간이 duration을 넘어가면 다시 0비슷하게 되돌려서 무한 재생되도록 함
			if (layers[1].elapsed_time >= duration) {
				layers[1].elapsed_time = fmodf(layers[1].elapsed_time, duration);
			}
		}
		else {
			if (elapsed < duration) {
				layers[1].weight += deltaTime * 10.0f;
			}
			else {	// fade out
				layers[1].weight -= deltaTime * 5.0f;
				if (layers[1].weight <= 0.0f) {
					layers[1].current_clip = "";
					layers[1].elapsed_time = 0.0f;
				}
			}
		}

		if (layers[1].weight > 1.0f) layers[1].weight = 1.0f;
		if (layers[1].weight < 0.0f) layers[1].weight = 0.0f;
	}
}

AnimationData CAnimatorComponent::GetAnimationData()
{
	AnimationData data{};
	auto& manager = CAnimationManager::GetInstance();

	// 베이스 애니메이션 (Layer 0)
	std::string baseClip = layers[0].current_clip;
	if (baseClip.empty()) return data;

	auto& animA = manager.GetClip(baseClip);
	data.start_offset_A = animA.start_matrix_offset;

	float relativeTimeA = layers[0].elapsed_time;
	data.cur_frame_A = (uint32_t)(relativeTimeA * 60.0f) % animA.total_frames;
	data.bone_count = animA.bone_count;

	// pitch 가중치 계산
	float pitchWeight{ -1.0f };
	bool isUp{};
	if (!up_clip.name.empty()) {
		// pitch 정규화
		float pitch = owner->pitch / 90.f;
		if(pitch <= 0.0f) isUp = true;
		pitchWeight = std::abs(pitch);
	}

	// 블렌딩 대상 결정
	// 상반신 액션(감정 표현 모션 등)
	if (!layers[1].current_clip.empty() && layers[1].weight > 0.0f) {
		// 상반신 액션 모드
		auto& animB = manager.GetClip(layers[1].current_clip);
		data.start_offset_B = animB.start_matrix_offset;

		float relativeTimeB = layers[1].elapsed_time;
		data.cur_frame_B = (uint32_t)(relativeTimeB * 60.0f) % animB.total_frames;

		data.blend_weight = layers[1].weight;
		data.mask_id = layers[1].mask_id;
	}
	else if (controller.IsBlending()) {	// blending 처리
		std::string clipB = controller.GetNextClip();
		if (!clipB.empty()) {
			auto& animB = manager.GetClip(clipB);
			data.start_offset_B = animB.start_matrix_offset;
			data.cur_frame_B = (uint32_t)(current_time * 60.0f) % animB.total_frames;
			data.blend_weight = controller.GetWeight();
			data.mask_id = -1;
		}
	}
	else if (pitchWeight > 0.0f) {
		if (isUp) data.start_offset_B = up_clip.start_matrix_offset;
		else data.start_offset_B = down_clip.start_matrix_offset;
		data.cur_frame_B = 0;
		data.blend_weight = pitchWeight;
		data.mask_id = 0;	// 상반신 마스크 ID 적용 (상체만 고개 들게)
	}
	else {
		data.blend_weight = 0.0f;
		data.mask_id = -1;
	}

	return data;
}

void CAnimatorComponent::Update(float deltaTime)
{
	if (owner == nullptr)
		return;

	current_time += deltaTime;

	// 애니메이션 상태 머신 update
	controller.Update(deltaTime);
	UpdateLayerWeights(deltaTime);

	layers[0].current_clip = controller.GetCurrentClip();
	layers[0].elapsed_time = controller.GetCurrentClipTime();
}
