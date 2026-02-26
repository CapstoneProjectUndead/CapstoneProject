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

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateLobby(CDescriptorHeapManager* heapManager)
{
	std::vector<std::shared_ptr<CObject>> objects;

	std::string fileName{ "../Modeling/lobby_uv.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	for (const auto& children : frameRoot->childrens) {
		if (children->mesh.positions.empty()) break;
		auto obj = std::make_shared<CObject>();
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
		if (children->name == "Wall") {
			std::unique_ptr< CColliderShape> shape = std::make_unique<CTriangleMeshShape>(children->collider.positions, children->collider.indices);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
		}
		else {
			if (children->name == "Floor" || children->name == "GroundPipe") {
				std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
				auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
				obj->SetComponent(boxCollider);
				CPhysicsManager::GetInstance().SetCollider(boxCollider);
			}
			else if (children->name == "Stone012") {	// 카운터
				std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(children->mesh.bounds.Extents.x, children->mesh.bounds.Center);
				auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
				obj->SetComponent(collider);
				CPhysicsManager::GetInstance().SetCollider(collider);
			}
			else if (!children->collider.positions.empty()) {
				std::unique_ptr< CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
				auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
				obj->SetComponent(collider);
				CPhysicsManager::GetInstance().SetCollider(collider);
			}
		}

		obj->Initialize(GET_DEVICE, GET_CMD_LIST);

		objects.push_back(std::move(obj));
	}


	return objects;
}

void CObjectFactory::CreateUndeadCharacter(std::shared_ptr<CPlayer> character, CDescriptorHeapManager* heapManager)
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
		switch (stringToMeshName(child->name)) {
		case MeshName::body:
			character->body_materials[0] = CreateUnit(resourceNames[0]);
			character->body_materials[1] = CreateUnit(resourceNames[1]);
			character->body_materials[1]->SetEnable(false);
			character->body_materials[2] = CreateUnit(resourceNames[2]);
			character->body_materials[2]->SetEnable(false);
			break;
		case MeshName::Bunny_ear:
		case MeshName::Bunny_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[2].push_back(meshComp);
			meshComp->SetEnable(false);
			break;
		case MeshName::Cat_ear:
		case MeshName::Cat_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[1].push_back(meshComp);
			meshComp->SetEnable(false);
			break;
		case MeshName::Dog_ear:
		case MeshName::Dog_tail:
			CreateUnit(resourceNames[3]);
			character->eartail_parts[0].push_back(meshComp);
			break;
		case MeshName::eyes:
			character->eyes_material[0] = CreateUnit(resourceNames[4]);
			character->eyes_material[1] = CreateUnit(resourceNames[5]);
			character->eyes_material[1]->SetEnable(false);
			character->eyes_material[2] = CreateUnit(resourceNames[6]);
			character->eyes_material[2]->SetEnable(false);
			break;
		case MeshName::mouse:
			character->mouth_material[0] = CreateUnit(resourceNames[7]);
			character->mouth_material[1] = CreateUnit(resourceNames[8]);
			character->mouth_material[1]->SetEnable(false);
			character->mouth_material[2] = CreateUnit(resourceNames[9]);
			character->mouth_material[2]->SetEnable(false);
			break;
		}
	}
	// ColliderComponent 생성
	std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.x, totalBounds.Center);
	auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);
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
	player->SetComponent(std::make_shared<CMovementComponent>());
	return player;
}

std::shared_ptr<CPlayer> CObjectFactory::CreatePlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CPlayer>();
	CreateUndeadCharacter(player, heapManager);
	player->SetComponent(std::make_shared<CMovementComponent>());
	return player;
}

void CObjectFactory::SetComponent(std::shared_ptr<CPlayer>& player)
{
	player->SetComponent(std::make_shared<CMovementComponent>());
}

CObjectFactory::MeshName CObjectFactory::stringToMeshName(const std::string& str)
{
	static const std::unordered_map<std::string, MeshName> table = {
	{"body", MeshName::body},
	{"Bunny_ear", MeshName::Bunny_ear},
	{"Bunny_tail", MeshName::Bunny_tail},
	{"Cat_ear", MeshName::Cat_ear},
	{"Cat_tail", MeshName::Cat_tail},
	{"Dog_ear", MeshName::Dog_ear},
	{"Dog_tail", MeshName::Dog_tail},
	{"eyes", MeshName::eyes},
	{"mouse", MeshName::mouse},
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : MeshName::Unknown;
}