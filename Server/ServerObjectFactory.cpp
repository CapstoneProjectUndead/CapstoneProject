#include "pch.h"
#include "ServerObjectFactory.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
#include "Movement.h"
#include "User.h"
#include "Room.h"
#include "Player.h"
#include "HumanMonster.h"

atomic<uint32> CServerObjectFactory::monster_id = 1001;


CServerObjectFactory::CServerObjectFactory()
{
}

CServerObjectFactory::~CServerObjectFactory()
{
}

shared_ptr<CPlayer> CServerObjectFactory::CreatePlayerTest(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user)
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	InitializeCharacter(player);

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

	return player;
}

shared_ptr<CPlayer> CServerObjectFactory::CreatePlayer(SCENE_TYPE sceneType, shared_ptr<Session> session, shared_ptr<CUser> user, shared_ptr<CRoom> room)
{
	shared_ptr<CPlayer> player = make_shared<CPlayer>();
	InitializeCharacter(player);

	// 유저를 약한 참조 (refcount 증가x)
	player->SetUser(user);

	// (플레이어 ID = 유저 ID)
	//player->SetID(user->GetUserID());

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

	// 방 인원 수 증가
	room->PlayerEnter();

	return player;
}

shared_ptr<CMonster> CServerObjectFactory::CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType)
{
	switch (monType)
	{
	case MON_TYPE::HUMAN_MONSTER:
	{
		auto humanMonster = make_shared<CHumanMonster>();
		InitializeCharacter(humanMonster);

		// monster 오브젝트 ID 설정
		uint32 id = monster_id.fetch_add(1);
		humanMonster->SetID(id);

		// monster가 속한 scene
		humanMonster->SetCurrentSceneType(sceneType);

		return humanMonster;
	}
		break;
	case MON_TYPE::ANIMAL_MONSTER:
		break;
	case MON_TYPE::GHOST:
		break;
	default:
		break;
	}

	return nullptr;
}

void CServerObjectFactory::InitializeCharacter(shared_ptr<CObject> object)
{
	// -------------------------------------
	// 플레이어에게 MovementComponent 달아주기
	// -------------------------------------
	object->SetComponent(std::make_shared<CMovementComponent>());

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
	CPhysicsManager::GetInstance().SetCollider(collider);

	object->UpdateWorldMatrix();
	collider->Update(0.0f);
}