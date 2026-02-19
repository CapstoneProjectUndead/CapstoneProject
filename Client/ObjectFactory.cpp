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
		matComp->SetMaterial(mat);

		// 3) MeshRendererComponent 생성
		auto meshRenderer = std::make_shared<CMeshRendererComponent>();
		obj->SetComponent(meshRenderer);
		meshRenderer->SetRenderUnit(meshComp.get(), matComp.get());

		
		if (children->name == "Floor") {
			// 4) ColliderComponent 생성
			std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
		}
		else if(children->name != "Wall"){
			// 4) ColliderComponent 생성
			std::unique_ptr< CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);

			if (!children->collider.positions.empty()) {
				auto debugMesh = std::make_shared<CMeshComponent>();
				obj->SetComponent(debugMesh);
				debugMesh->SetMeshFromFile<CVertex>(GET_DEVICE, GET_CMD_LIST, children->collider);
				meshRenderer->SetRenderUnit(debugMesh.get());
			}
		}

		obj->Initialize(GET_DEVICE, GET_CMD_LIST);

		objects.push_back(std::move(obj));
	}

	return objects;
}


void CObjectFactory::CreateCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager)
{
	std::string fileName{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	// 1) Mesh 로드 + totalBounds 계산
	BoundingBox totalBounds;
	bool firstBounds = true;

	// MeshRendererComponent 생성
	auto renderer = std::make_shared<CMeshRendererComponent>();
	character->SetComponent(renderer);

	for (const auto& child : frameRoot->childrens) {
		if (child->mesh.positions.empty())
			continue;

		RenderUnit renderUnit{};
		// mesh component
		auto meshComp = std::make_shared<CMeshComponent>();
		renderUnit.mesh = meshComp.get();
		character->SetComponent(meshComp);
		meshComp->SetMeshFromFile<CSkinnedVertex>(GET_DEVICE, GET_CMD_LIST, child);
		// bounds merge
		if (firstBounds) {
			totalBounds = child->mesh.bounds;
			firstBounds = false;
		}
		else {
			BoundingBox::CreateMerged(totalBounds, totalBounds, child->mesh.bounds);
		}

		// MaterialComponent 생성
		std::string name{ child->mesh.materials[0].albedoMap };
		if (!name.empty()) {
			auto matComp = std::make_shared<CMaterialComponent>();
			character->SetComponent(matComp);

			std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
			std::shared_ptr<CMaterial> mat = matManager.GetMeterial(name, tex);
			matComp->SetMaterial(mat);
			renderUnit.material = matComp.get();
		}

		// renderUnit set
		renderer->SetRenderUnit(renderUnit);
	}
	// 4) ColliderComponent 생성
	std::unique_ptr< CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.x, totalBounds.Center);
	auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);
	character->SetComponent(collider);
	CPhysicsManager::GetInstance().SetCollider(collider);

	// 4) Animator
	auto animator = std::make_shared<CAnimatorComponent>();
	animator->Initialize(fileName, "../Modeling/undead_ani.bin");
	animator->Play("Ganga_walk");
	character->SetComponent(animator);
	character->SetShdaer("skinning");

	// Movement
	character->SetComponent(std::make_shared<CMovementComponent>());
}

std::shared_ptr<CMyPlayer> CObjectFactory::CreateMyPlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CMyPlayer>();
	CreateCharacter(player, heapManager);
	player->Initialize(GET_DEVICE, GET_CMD_LIST);
	return player;
}

std::shared_ptr<CPlayer> CObjectFactory::CreatePlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CPlayer>();
	CreateCharacter(player, heapManager);
	player->Initialize(GET_DEVICE, GET_CMD_LIST);
	return player;
}
