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

void CObjectFactory::LoadFrameNode(CDescriptorHeapManager* heapManager, std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node)
{
	if (node->mesh.positions.empty()) return;

	auto obj = std::make_shared<CObject>();
	// 1) MeshComponent 생성
	auto meshComp = std::make_shared<CMeshComponent>();
	obj->SetComponent(meshComp);
	meshComp->SetMeshFromFile<CMatVertex>(GET_DEVICE, GET_CMD_LIST, node);
	obj->world_matrix = node->localMatrix;

	// 2) MaterialComponent 생성
	auto matComp = std::make_shared<CMaterialComponent>();
	obj->SetComponent(matComp);

	std::string name{ node->mesh.materials[0].albedoMap };
	if (!name.empty()) {
		std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
		std::shared_ptr<CMaterial> mat = matManager.GetMeterial(name, tex);
		mat->material.albedo = node->mesh.materials[0].albedoColor;
		mat->material.glossiness = node->mesh.materials[0].glossiness;
		matComp->SetMaterial(mat);
	}

	// 3) MeshRendererComponent 생성
	auto meshRenderer = std::make_shared<CMeshRendererComponent>();
	obj->SetComponent(meshRenderer);
	meshRenderer->SetRenderUnit(meshComp.get(), matComp.get());

	// ColliderComponent 생성
	std::unique_ptr< CColliderShape> shape = std::make_unique<CBoxShape>(node->mesh.bounds.Extents, node->mesh.bounds.Center);
	auto boxCollider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
	CollisionFilter filter;
	filter.category = EColLayer::OBJECT;
	filter.mask = EColLayer::PLAYER;
	boxCollider->SetFillter(filter);
	obj->SetComponent(boxCollider);
	//CPhysicsManager::GetInstance().SetCollider(boxCollider);

	obj->Initialize(GET_DEVICE, GET_CMD_LIST);

	objects.emplace(node->name, std::move(obj));
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateLobby(CDescriptorHeapManager* heapManager)
{
	std::vector<std::shared_ptr<CObject>> objects;

	std::string fileName{ "../Modeling/lobby_0305.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	for (const auto& children : frameRoot->childrens) {
		if (children->mesh.positions.empty()) continue;
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

void CObjectFactory::LoadGameScene(CDescriptorHeapManager* heapManager)
{
	if (!prototypes.empty()) return;

	std::string fileName{ "../Modeling/all_map.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	LoadFrameNode(heapManager, prototypes, frameRoot);
	for (const auto& children : frameRoot->childrens) {
		LoadFrameNode(heapManager, prototypes, children);
	}
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateGameScene(CDescriptorHeapManager* heapManager)
{
	if (prototypes.empty()) LoadGameScene(heapManager);

	std::vector<std::shared_ptr<CObject>> objects;
	std::vector<MapGenerator::InstanceData> instData = MapGenerator::Generate3DMap();
	for (const auto& inst : instData) {
		std::string typeName{ GameSceneTypeToString(inst.type) };
		auto meshComp = prototypes[typeName]->GetComponent<CMeshComponent>();
		auto matComp = prototypes[typeName]->GetComponent<CMaterialComponent>();
		auto collider = prototypes[typeName]->GetComponent<CColliderComponent>();

		// 위치/크기 정보를 행렬로 변환하여 추가
		// * XMMatrixScaling(inst.scale.x, inst.scale.y, inst.scale.z)
		XMMATRIX world = XMLoadFloat4x4(&prototypes[typeName]->world_matrix) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);

		objects.push_back(std::make_shared<CObject>());
		XMStoreFloat4x4(&objects.back()->world_matrix, world);

		// 인스턴스 렌더러에 위치와 리소스 정보 등록
		CInstRenderer::GetInstance().AddInstance(
			meshComp->GetMesh().get(),
			matComp,
			objects.back()->world_matrix
		);
		objects.back()->SetShdaer("inst");
		auto copyCollider = std::make_shared< CColliderComponent>(*collider);
		objects.back()->SetComponent(copyCollider);
		CPhysicsManager::GetInstance().SetCollider(copyCollider);
	}
	CInstRenderer::GetInstance().Initialize(GET_DEVICE, GET_CMD_LIST, instData.size());
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

std::string CObjectFactory::GameSceneTypeToString(const MapGenerator::EModelType& type)
{
	static const std::unordered_map<MapGenerator::EModelType, std::string> table = {
		{ MapGenerator::EModelType::ROAD,					"park_road" },
		{ MapGenerator::EModelType::PARK_GREEN,				"park_green" },
		{ MapGenerator::EModelType::VILLAGE_ROAD,			"village_road" },
		{ MapGenerator::EModelType::HOUSE_INNTER,			"house_place" },

		{ MapGenerator::EModelType::WALL,					"seesaw001" },

		{ MapGenerator::EModelType::HOUSE_WALL_CORNER,		"wall_2001" },
		{ MapGenerator::EModelType::HOUSE_WALL_STRAIGHT,	"wall_1002" },
		{ MapGenerator::EModelType::HOUSE_WALL_EMPTY,		"wall_1003" },
		/*{ MapGenerator::EModelType::WAREHOUSE,				"wall_1002" },
		{ MapGenerator::EModelType::STORE,					"vending_machine001" },*/
		{ MapGenerator::EModelType::DOOR,					"wall_1_door001" },

		{ MapGenerator::EModelType::KIOSK,					"table001" },
		{ MapGenerator::EModelType::TREE,					"pinetree" },
		{ MapGenerator::EModelType::TREASURE,				"trashcan001" },
		{ MapGenerator::EModelType::BENCH,					"park_bench002" },
	};

	auto it = table.find(type);
	return (it != table.end()) ? it->second : "park_road";
}