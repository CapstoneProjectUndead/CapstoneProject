#include "pch.h"
#include "ServerObjectFactory.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
#include "Movement.h"
#include "User.h"
#include "Room.h"
#include "Player.h"
#include "Inventory.h"
#include "HumanMonster.h"
#include "Ghost.h"
#include "AIComponent.h"
#include "AIStates.h"

atomic<uint32> CServerObjectFactory::monster_id_generator = 1001;


CServerObjectFactory::CServerObjectFactory()
{
}

CServerObjectFactory::~CServerObjectFactory()
{
}

shared_ptr<CPlayer> CServerObjectFactory::CreatePlayerTest(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CPhysicsManager> physicsManager)
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	InitializeUndeadCharacter(player, physicsManager);

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// (플레이어 ID = 유저 ID)
	player->SetID(user->GetUserID());

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어가 속한 scene 설정
	player->SetCurrentSceneType(sceneType);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 인벤토리 생성
	shared_ptr<CInventory> inventory = make_shared<CInventory>(player);
	player->SetInventory(inventory);

	return player;
}

shared_ptr<CPlayer> CServerObjectFactory::CreatePlayer(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager)
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	InitializeUndeadCharacter(player, physicsManager);

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// (플레이어 ID = 유저 ID)
	player->SetID(user->GetUserID());

	// 세션도 약한 참조 (refcount 증가x)
	player->SetSession(user->GetSession());

	// 플레이어가 속한 scene 설정
	player->SetCurrentSceneType(sceneType);

	// 유저가 자신의 플레이어를 참조 (refcount 증가)
	user->SetPlayer(player);

	// 지금 플레이어가 속한 방
	player->SetRoom(room);

	// 지금 플레이어가 속한 방ID
	player->SetRoomID(room->GetRoomID());

	// 인벤토리 생성
	shared_ptr<CInventory> inventory = make_shared<CInventory>(player);
	player->SetInventory(inventory);

	// 방 인원 수 증가
	room->PlayerEnter();

	return player;
}

shared_ptr<CMonster> CServerObjectFactory::CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType, shared_ptr<CRoom> room, shared_ptr<CPhysicsManager> physicsManager)
{
	shared_ptr<CMonster> monster;

	// AI 생성
	auto AIComp = std::make_shared<CAIComponent>();

	switch (monType)
	{
	case MON_TYPE::HUMAN_MONSTER:
	{
		monster = make_shared<CHumanMonster>();
		InitializeHumanMonster(monster, physicsManager);
	}
		break;
	case MON_TYPE::ANIMAL_MONSTER:
		break;
	case MON_TYPE::GHOST:
	{
		monster = make_shared<CGhost>();
		InitializeGhost(monster, physicsManager);
	}
		break;
	default:
		return nullptr;
		break;
	}

	// monster 오브젝트 ID 설정
	uint32 id = monster_id_generator.fetch_add(1);
	monster->SetID(id);

	// monster가 속한 scene
	monster->SetCurrentSceneType(sceneType);

	// IDLE, PATROL, TRACE, ATTACK 상태는 모든 몬스터가 공통으로 가진다.
	AIComp->AddState(std::make_shared<CIdleState>());
	AIComp->AddState(std::make_shared<CPatrolState>());
	AIComp->AddState(std::make_shared<CTraceState>());
	AIComp->AddState(std::make_shared<CAttackState>());

	// 항상 IDLE 로 시작.
	AIComp->SetState(AI_STATE::MONSTER_IDLE);

	// AI 컴포넌트 Monster에 등록
	monster->SetComponent(AIComp);

	// Movement 컴포넌트 추가. (순서가 중요. AI -> Movement 순서로 가야함.)
	shared_ptr<CMovementComponent> movementComponent = std::make_shared<CMovementComponent>();
	movementComponent->SetPhysicsManager(physicsManager);
	if (monType == MON_TYPE::GHOST)
		movementComponent->SetCollisionMask(EColLayer::WALL);
	monster->SetComponent(movementComponent);

	// Monster가 속한 Room 
	monster->SetRoom(room);
	monster->SetRoomID(room->GetRoomID());

	return monster;
}

void CServerObjectFactory::InitializeCharacter(const std::string& fileName, shared_ptr<CObject>& object, shared_ptr<CPhysicsManager>& physicsManager, bool isPlayer, EColLayer colMask)
{
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	ProcessNode(physicsManager, object, frameRoot, isPlayer, colMask);
	for (const auto& child : frameRoot->childrens) {
		ProcessNode(physicsManager, object, child, isPlayer, colMask);
	}
}

void CServerObjectFactory::InitializeUndeadCharacter(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager)
{
	// -------------------------------------
	// 플레이어에게 MovementComponent 달아주기
	// -------------------------------------
	if (object->GetObjectType() == OBJECT_TYPE::PLAYER) {

		shared_ptr<CMovementComponent> movementComponent = std::make_shared<CMovementComponent>();

		// 3월 21일 (추가) MovementComponent는 physics_manager를 들고있는게 편하다. 그래서 여기서 추가한다.
		movementComponent->SetPhysicsManager(physicsManager);

		object->SetComponent(movementComponent);
	}

	// -----------------------------------
	// 플레이어에게 충돌체(Collider) 달아주기
	// -----------------------------------
	std::string fileName{ "../Modeling/undead_char_0412.bin" };
	InitializeCharacter(fileName, object, physicsManager, true);
}

void CServerObjectFactory::InitializeHumanMonster(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager)
{
	std::string fileName{ "../Modeling/Human_monster.bin" };
	InitializeCharacter(fileName, object, physicsManager, false);
}

void CServerObjectFactory::InitializeGhost(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager)
{
	std::string fileName{ "../Modeling/Ghost3.bin" };
	InitializeCharacter(fileName, object, physicsManager, false,
		static_cast<EColLayer>(EColLayer::WALL | EColLayer::GROUND));
}

void CServerObjectFactory::ProcessNode(shared_ptr<CPhysicsManager> physicsManager, shared_ptr<CObject>& object, const std::unique_ptr<CGeometryLoader::FrameNode>& node, bool isPlayer, EColLayer colMask)
{
	EColLayer category = isPlayer ? EColLayer::PLAYER : EColLayer::CHARACTER;
	AddCollider(physicsManager, object, node, category, colMask);
}

void CServerObjectFactory::AddCollider(shared_ptr<CPhysicsManager> physicsManager, std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EColLayer category, EColLayer mask)
{
	if (!node) return;

	// Mesh Collider
	if (!node->mesh_colliders.empty()) {
		for (const auto& mc : node->mesh_colliders) {
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(mc.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
			collider->SetFillter({ category, mask });
			obj->SetComponent(collider);
			collider->Update(0.0f);
			physicsManager->SetCollider(collider);
		}
	}

	// Box Colliders
	for (const auto& box : node->box_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(box.size, box.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		collider->Update(0.0f);
		physicsManager->SetCollider(collider);
	}

	// Sphere Colliders
	for (const auto& sphere : node->sphere_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CSphereShape>(sphere.radius, sphere.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		collider->Update(0.0f);
		physicsManager->SetCollider(collider);
	}

	// Capsule Colliders
	for (const auto& cap : node->capsule_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CCapsuleShape>(cap.radius, cap.height, cap.direction, cap.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		collider->Update(0.0f);
		physicsManager->SetCollider(collider);
	}
}