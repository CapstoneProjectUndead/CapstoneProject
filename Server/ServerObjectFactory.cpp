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
	InitializeCharacter(player, physicsManager);

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
	InitializeCharacter(player, physicsManager);

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
		InitializeCharacter(monster, physicsManager);
	}
		break;
	case MON_TYPE::ANIMAL_MONSTER:
		break;
	case MON_TYPE::GHOST:
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
	monster->SetComponent(movementComponent);

	// Monster가 속한 Room 
	monster->SetRoom(room);
	monster->SetRoomID(room->GetRoomID());

	return monster;
}

void CServerObjectFactory::InitializeCharacter(shared_ptr<CObject> object, shared_ptr<CPhysicsManager> physicsManager)
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
	std::string fileName{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	// Mesh 로드 + totalBounds 계산
	BoundingBox totalBounds;
	bool firstBounds = true;

	for (const auto& child : frameRoot->childrens) {
		if (child->mesh.positions.empty())
			continue;
		// bounds merge
		if (firstBounds) {
			totalBounds = child->mesh.bounds;
			firstBounds = false;
		}
		else {
			BoundingBox::CreateMerged(totalBounds, totalBounds, child->mesh.bounds);
		}
	}

	// ColliderComponent 생성/ filter 설정
	std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.y, totalBounds.Center);
	auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);
	CollisionFilter filter;
	filter.category = EColLayer::PLAYER;
	filter.mask = EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND;
	collider->SetFillter(filter);
	object->SetComponent(collider);
	physicsManager->SetCollider(collider);

	object->UpdateWorldMatrix();
	collider->Update(0.0f);
}