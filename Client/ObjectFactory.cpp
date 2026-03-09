#include "stdafx.h"
#include "ObjectFactory.h"

// Component
#include "MeshComponent.inl"
#include "Collider.h"
#include "Mesh.h"
#include "Animator.h"
#include "Movement.h"

#include "Shader.h"
#include "PhysicsManager.h"
#include "GameFramework.h"
#include "Character.h"
#include "MyPlayer.h"
#include "HumanMonster.h"
#include "AIComponent.h"
#include "IdleState.h"
#include "PatrolState.h"
#include "TraceState.h"
#include "AttackState.h"

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateLobby(CDescriptorHeapManager* heapManager)
{
	std::vector<std::shared_ptr<CObject>> objects;

	std::string fileName{ "../Modeling/lobby_uv.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	for (const auto& children : frameRoot->childrens) {
		if (children->mesh.positions.empty()) break;
		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
		// 1) MeshComponent 생성
		auto meshComp = std::make_shared<CMeshComponent>();
		obj->SetComponent(meshComp);
		meshComp->SetMeshFromFile<CMatVertex>(GET_DEVICE, GET_CMD_LIST, children);
		obj->world_matrix = children->localMatrix;

		// 2) MaterialComponent 생성
		auto matComp = std::make_shared<CMaterialComponent>();
		obj->SetComponent(matComp);

		std::string name{ children->mesh.materials[0].albedoMap };
		std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
		std::shared_ptr<CMaterial> mat = matManager.GetMeterial(name, tex);
		mat->material.albedo = children->mesh.materials[0].albedoColor;
		mat->material.glossiness = children->mesh.materials[0].glossiness;
		matComp->SetMaterial(mat);

		// 3) MeshRendererComponent 생성
		auto meshRenderer = std::make_shared<CMeshRendererComponent>();
		obj->SetComponent(meshRenderer);
		meshRenderer->SetRenderUnit(meshComp.get(), matComp.get());

		// ColliderComponent 생성
		// filter(지형만 별도처리)
		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;
		switch (stringToLobbyMeshName(children->name)) {
		case LobbyMeshName::Wall:
		{
			std::unique_ptr< CColliderShape> shape = std::make_unique<CConcaveMeshShape>(children->collider.positions, children->collider.indices);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			CollisionFilter filter;
			filter.category = EColLayer::WALL;
			filter.mask = EColLayer::PLAYER;
			collider->SetFillter(filter);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		case LobbyMeshName::Floor:
		{
			std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			CollisionFilter filter;
			filter.category = EColLayer::GROUND;
			filter.mask = EColLayer::PLAYER;
			boxCollider->SetFillter(filter);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
			break;
		}
		case LobbyMeshName::GroundPipe:
		{
			std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			boxCollider->SetFillter(filter);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
			break;
		}
		case LobbyMeshName::Counter:
		{
			std::unique_ptr< CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			collider->SetFillter(filter);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		case LobbyMeshName::Unknown:
		{
			if (children->collider.positions.empty()) break;
			std::unique_ptr< CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			collider->SetFillter(filter);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		}

		obj->Initialize(GET_DEVICE, GET_CMD_LIST);

		objects.push_back(std::move(obj));
	}


	return objects;
}

void CObjectFactory::CreateUndeadCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager)
{
	std::string fileName{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	// material 미리 Load
	std::vector<std::string> resourceNames = {
		"body_ganga", "body_nyao", "body_toto",
		"eartail",
		"eyes_ganga", "eyes_nyao", "eyes_toto",
		"mouse_ganga", "mouse_nyao", "mouse_toto"
	};
	for (const std::string& name : resourceNames) {
		std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
		matManager.LoadMeterial(name, tex);
	}

	// Mesh 로드 + totalBounds 계산
	BoundingBox totalBounds;
	bool firstBounds = true;

	// MeshRendererComponent 생성
	auto renderer = std::make_shared<CMeshRendererComponent>();
	character->SetComponent(renderer);

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

		// mesh component
		auto meshComp = std::make_shared<CMeshComponent>();
		character->SetComponent(meshComp);
		meshComp->SetMeshFromFile<CSkinnedVertex>(GET_DEVICE, GET_CMD_LIST, child);

		// material 생성 후 renderer->SetRenderUnit 수행
		auto CreateUnit = [&](std::string texName) {
			auto matComp = std::make_shared<CMaterialComponent>();
			auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
			auto mat = matManager.GetMeterial(texName, tex);
			matComp->SetMaterial(mat);
			character->SetComponent(matComp);

			RenderUnit unit;
			unit.mesh = meshComp.get();
			unit.material = matComp.get();
			renderer->SetRenderUnit(unit);

			return matComp; // 나중에 껐다 켜기 위해 반환
		};

		// 0: dog, 1: cat, 2: buddy
		// 처음에 강아지만 enable true
		switch (stringToUndeadMeshName(child->name)) {
		case UndeadMeshName::body:
			character->body_materials[0] = CreateUnit(resourceNames[0]);
			character->body_materials[1] = CreateUnit(resourceNames[1]);
			character->body_materials[1]->SetEnable(false);
			character->body_materials[2] = CreateUnit(resourceNames[2]);
			character->body_materials[2]->SetEnable(false);
			break;
		case UndeadMeshName::Bunny_ear:
		case UndeadMeshName::Bunny_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[2].push_back(meshComp);
			meshComp->SetEnable(false);
			break;
		case UndeadMeshName::Cat_ear:
		case UndeadMeshName::Cat_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[1].push_back(meshComp);
			meshComp->SetEnable(false);
			break;
		case UndeadMeshName::Dog_ear:
		case UndeadMeshName::Dog_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[0].push_back(meshComp);
			break;
		case UndeadMeshName::eyes:
			character->eyes_material[0] = CreateUnit(resourceNames[4]);
			character->eyes_material[1] = CreateUnit(resourceNames[5]);
			character->eyes_material[1]->SetEnable(false);
			character->eyes_material[2] = CreateUnit(resourceNames[6]);
			character->eyes_material[2]->SetEnable(false);
			break;
		case UndeadMeshName::mouse:
			character->mouth_material[0] = CreateUnit(resourceNames[7]);
			character->mouth_material[1] = CreateUnit(resourceNames[8]);
			character->mouth_material[1]->SetEnable(false);
			character->mouth_material[2] = CreateUnit(resourceNames[9]);
			character->mouth_material[2]->SetEnable(false);
			break;
		}
	}
	// ColliderComponent 생성/ filter 설정
	std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.y, totalBounds.Center);
	auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);
	CollisionFilter filter;
	filter.category = EColLayer::PLAYER;
	filter.mask = EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND;
	collider->SetFillter(filter);
	character->SetComponent(collider);
	CPhysicsManager::GetInstance().SetCollider(collider);

	// Animator
	auto animator = std::make_shared<CAnimatorComponent>();

	animator->Initialize(fileName, "../Modeling/undead_ani.bin");
	character->SetComponent(animator);
	character->SetShdaer("skinning");

	character->Initialize(GET_DEVICE, GET_CMD_LIST);
}

std::shared_ptr<CMyPlayer> CObjectFactory::CreateMyPlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CMyPlayer>();
	CreateUndeadCharacter(player, heapManager);

	// 싱글 모드에서만 CMovementComponent를 추가한다.
	if (g_is_single) {
		player->SetComponent(std::make_shared<CMovementComponent>());
	}

	return player;
}

std::shared_ptr<CPlayer> CObjectFactory::CreatePlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CPlayer>();
	CreateUndeadCharacter(player, heapManager);
	return player;
}

std::shared_ptr<CHumanMonster> CObjectFactory::CreateHumanMonster(CDescriptorHeapManager* heapManager)
{
	auto humanMonster = std::make_shared<CHumanMonster>();
	CreateUndeadCharacter(humanMonster, heapManager);

	// AI 생성
	auto AIComp = std::make_shared<CAIComponent>();

	// HumanMonster의 상태들을 AI에 추가
	AIComp->AddState(std::make_shared<CIdleState>());
	AIComp->AddState(std::make_shared<CPatrolState>());
	AIComp->AddState(std::make_shared<CTraceState>());
	AIComp->AddState(std::make_shared<CAttackState>());
	AIComp->SetState(AI_STATE::MONSTER_IDLE);

	// AI 컴포넌트 HumanMonster에 등록
	humanMonster->SetComponent(AIComp);

	// Movement 컴포넌트 추가. (순서가 중요. AI -> Movement 순서로 가야함.)
	humanMonster->SetComponent(std::make_shared<CMovementComponent>());

	return humanMonster;
}

void CObjectFactory::SetComponent(std::shared_ptr<CPlayer>& player)
{
	player->SetComponent(std::make_shared<CMovementComponent>());
}

CObjectFactory::UndeadMeshName CObjectFactory::stringToUndeadMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, UndeadMeshName> table = {
		{"body", UndeadMeshName::body},
		{"Bunny_ear", UndeadMeshName::Bunny_ear},
		{"Bunny_tail", UndeadMeshName::Bunny_tail},
		{"Cat_ear", UndeadMeshName::Cat_ear},
		{"Cat_tail", UndeadMeshName::Cat_tail},
		{"Dog_ear", UndeadMeshName::Dog_ear},
		{"Dog_tail", UndeadMeshName::Dog_tail},
		{"eyes", UndeadMeshName::eyes},
		{"mouse", UndeadMeshName::mouse},
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : UndeadMeshName::Unknown;
}

CObjectFactory::LobbyMeshName CObjectFactory::stringToLobbyMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, LobbyMeshName> table = {
		{"Wall", LobbyMeshName::Wall},
		{"Floor", LobbyMeshName::Floor},
		{"GroundPipe", LobbyMeshName::GroundPipe},
		{"Stone012", LobbyMeshName::Counter}
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}
