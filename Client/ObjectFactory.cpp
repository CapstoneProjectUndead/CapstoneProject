#include "stdafx.h"
#include "ObjectFactory.h"
#include "ItemFactory.h"

// Component
#include "MeshComponent.inl"
#include "Collider.h"
#include "Mesh.h"
#include "Animator.h"
#include "Movement.h"

#include "PhysicsManager.h"
#include "GameFramework.h"
#include "MyPlayer.h"
#include "HumanMonster.h"
#include "AIComponent.h"
#include "AIStates.h"
#include "ItemFinder.h"
#include "MapUtils.h"
#include "Inventory.h"

uint32 CObjectFactory::s_monster_id_generator = 1001;

std::shared_ptr<CMaterial> CObjectFactory::GetMaterial(CDescriptorHeapManager* heapManager, const std::string& name)
{
	std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
	std::shared_ptr<CMaterial> mat = matManager.GetMaterial(name, tex);

	return mat;
}

void CObjectFactory::LoadFrameNode(CDescriptorHeapManager* heapManager, std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	if (node->mesh.positions.empty()) return;

	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
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
		std::shared_ptr<CMaterial> mat = matManager.GetMaterial(name, tex);
		mat->material.albedo = node->mesh.materials[0].albedoColor;
		mat->material.glossiness = node->mesh.materials[0].glossiness;
		matComp->SetMaterial(mat);
	}

	// 3) MeshRendererComponent 생성
	auto meshRenderer = std::make_shared<CMeshRendererComponent>();
	obj->SetComponent(meshRenderer);
	meshRenderer->SetRenderUnit(meshComp.get(), matComp.get());

	auto SetColliderComp = [&node, obj](std::unique_ptr<CColliderShape>& shape) {
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;
		collider->SetFillter(filter);
		obj->SetComponent(collider);
		};

	// ColliderComponent 생성
	if (g_is_single) {
		bool isRoad = node->name == "park_road" || node->name == "village_road" || node->name == "park_green" || node->name == "house_place";
		if (!node->collider.positions.empty()) {
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(node->collider.positions);
			SetColliderComp(shape);
		}
		else if (isRoad) {
			std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(node->mesh.bounds.Extents, node->mesh.bounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
			CollisionFilter filter;
			filter.category = EColLayer::GROUND;
			filter.mask = EColLayer::PLAYER;
			boxCollider->SetFillter(filter);
			obj->SetComponent(boxCollider);
		}
	}
	
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
		std::shared_ptr<CMaterial> mat = matManager.GetMaterial(name, tex);
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
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConcaveMeshShape>(children->collider.positions, children->collider.indices);
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
			std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
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
			std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(children->mesh.bounds.Extents, children->mesh.bounds.Center);
			auto boxCollider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			boxCollider->SetFillter(filter);
			obj->SetComponent(boxCollider);
			CPhysicsManager::GetInstance().SetCollider(boxCollider);
			break;
		}
		case LobbyMeshName::Unknown:
		{
			if (children->collider.positions.empty()) break;
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(children->collider.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
			collider->SetFillter(filter);
			obj->SetComponent(collider);
			CPhysicsManager::GetInstance().SetCollider(collider);
			break;
		}
		}

		// 디버깅용으로 저장
		obj->name = children->name;

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

	// 맵 데이터를 순회하며 보물 좌표만 빼오기
	for (const auto& inst : instData) {
		if (inst.type == MapGenerator::EModelType::TREASURE) {
			treasures.push_back(TreasureInfo{ inst.position });
		}
	}

	for (const auto& inst : instData) {
		for (const std::string& typeName : GameSceneTypeToString(inst.type)) {

			EModelVariant model = PickRandomVariant(typeName);
			if (model == EModelVariant::NONE) 
				continue;

			std::string meshName = GetVariantFileName(model);

			auto proto = prototypes[meshName];
			auto meshComp = proto->GetComponent<CMeshComponent>();
			auto matComp = proto->GetComponent<CMaterialComponent>();
			auto collider = proto->GetComponent<CColliderComponent>();
			auto meshRenderer = std::make_shared<CMeshRendererComponent>();

			// 위치/크기 정보를 행렬로 변환하여 추가
			auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

			XMMATRIX world = XMLoadFloat4x4(&proto->world_matrix) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
			XMStoreFloat4x4(&obj->world_matrix, world);

			RenderUnit unit;
			unit.mesh = meshComp;
			unit.material = matComp;
			meshRenderer->SetRenderUnit(unit);
			
			obj->SetComponent(meshRenderer);

			// collider copy(잔디, 돌은 필요X)
			std::string base{ typeName };
			std::erase_if(base, ::isdigit);
			// 길이면 boxShape으로 새로 생성
			if ((base != "grass" && base != "stone" && collider)) {
				auto copyCollider = std::make_shared<CColliderComponent>(*collider);
				obj->SetComponent(copyCollider);
				CPhysicsManager::GetInstance().SetCollider(copyCollider);
			}
			obj->SetShdaer("inst");

			objects.push_back(obj);
		}
	}
	return objects;
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateGameSceneByServer(CDescriptorHeapManager* heapManager, const std::vector<MapGenerator::InstanceData>& instanceData)
{
	if (prototypes.empty()) LoadGameScene(heapManager);

	std::vector<std::shared_ptr<CObject>> objects;

	// 맵 데이터를 순회하며 보물 좌표만 빼오기
	treasures.clear();
	for (const auto& inst : instanceData) {
		if (inst.type == MapGenerator::EModelType::TREASURE) {
			treasures.push_back(TreasureInfo{ inst.position });
		}
	}

	for (const auto& inst : instanceData) {

		std::string meshName = GetVariantFileName(inst.model);

		auto proto = prototypes[meshName];
		auto meshComp = proto->GetComponent<CMeshComponent>();
		auto matComp = proto->GetComponent<CMaterialComponent>();

		// 위치/크기 정보를 행렬로 변환하여 추가
		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

		XMMATRIX world = XMLoadFloat4x4(&proto->world_matrix) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
		XMStoreFloat4x4(&obj->world_matrix, world);

		obj->SetShdaer("inst");

		objects.push_back(obj);
	}
	return objects;
}

void CObjectFactory::InitializeCharacterComponents(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager,
	const std::string& modelFileName, const std::string& animFileName,
	std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor, bool isPlayer)
{
	auto frameRoot = CGeometryLoader::LoadGeometry(modelFileName);
	if (!frameRoot) return;

	// Renderer 및 기초 컴포넌트 설정
	auto renderer = std::make_shared<CMeshRendererComponent>();
	character->SetComponent(renderer);

	BoundingBox totalBounds;
	bool firstBounds = true;

	// 메쉬 노드 순회 및 파츠 처리
	for (const auto& child : frameRoot->childrens) {
		if (child->mesh.positions.empty()) continue;
		character->world_matrix = child->localMatrix;

		// Bounds 계산
		if (firstBounds) { totalBounds = child->mesh.bounds; firstBounds = false; }
		else { BoundingBox::CreateMerged(totalBounds, totalBounds, child->mesh.bounds); }

		// 공통 MeshComponent 생성
		auto meshComp = std::make_shared<CMeshComponent>();
		character->SetComponent(meshComp);
		if(isPlayer)
			meshComp->SetMeshFromFile<CSkinnedVertex>(GET_DEVICE, GET_CMD_LIST, child);
		else
			meshComp->SetMeshFromFile<CMatVertex>(GET_DEVICE, GET_CMD_LIST, child);

		// 모델마다 다른 상세 로직(머티리얼, 특정 파츠 분류)은 외부에서 주입받은 함수로 처리
		if (partProcessor) {
			partProcessor(child.get(), meshComp, renderer);
		}
	}

	// Collider 설정 (싱글 전용)
	if (g_is_single) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CSphereShape>(totalBounds.Extents.y, totalBounds.Center);
		auto collider = std::make_shared<CColliderComponent>(shape, totalBounds);

		CollisionFilter filter;
		if (isPlayer)
			filter.category = EColLayer::PLAYER;
		else
			filter.category = EColLayer::OBJECT;

		filter.mask = EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND;
		collider->SetFillter(filter);

		character->SetComponent(collider);
		CPhysicsManager::GetInstance().SetCollider(collider);
	}

	// 애니메이터 설정
	if (!animFileName.empty()) {
		auto animator = std::make_shared<CAnimatorComponent>();
		animator->Initialize(modelFileName, animFileName);
		character->SetComponent(animator);
	}

	character->SetShdaer("inst");
	character->Initialize(GET_DEVICE, GET_CMD_LIST);
}

void CObjectFactory::CreateUndeadCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager)
{
	std::string fileName{ "../Modeling/undead_char.bin" };

	// material 미리 Load
	std::vector<std::string> resourceNames = {
		"body_ganga", "body_nyao", "body_toto",
		"eartail",
		"eyes_ganga", "eyes_nyao", "eyes_toto",
		"mouse_ganga", "mouse_nyao", "mouse_toto"
	};
	for (const std::string& name : resourceNames) {
		std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
		matManager.LoadMaterial(name, tex);
	}

	auto undeadProcessor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
		// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
		auto CreateUnit = [&](const std::string& texName) {
			auto matComp = std::make_shared<CMaterialComponent>();
			auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
			auto mat = matManager.GetMaterial(texName, tex);
			matComp->SetMaterial(mat);
			character->SetComponent(matComp);

			RenderUnit unit{ meshComp.get(), matComp.get() };
			renderer->SetRenderUnit(unit);
			return matComp;
			};

		// 0: dog, 1: cat, 2: buddy
		// 처음에 강아지만 enable true
		switch (stringToUndeadMeshName(node->name)) {
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
	};

	InitializeCharacterComponents(
		character,
		heapManager,
		fileName,
		"../Modeling/undead_ani.bin",
		undeadProcessor,
		true
	);
}

std::shared_ptr<CCharacter> CObjectFactory::CreateReaper(CDescriptorHeapManager* heapManager)
{
	std::shared_ptr<CCharacter> character = std::make_shared<CCharacter>(OBJECT_TYPE::STATIC_OBJECT);

	std::string fileName{ "../Modeling/Reaper.bin" };

	auto undeadProcessor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
			// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
			auto CreateUnit = [&](const std::string& texName) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex);
				matComp->SetMaterial(mat);
				character->SetComponent(matComp);

				RenderUnit unit{ meshComp.get(), matComp.get() };
				renderer->SetRenderUnit(unit);
				return matComp;
				};

			CreateUnit(node->mesh.materials[0].albedoMap);
		};

	InitializeCharacterComponents(
		character,
		heapManager,
		fileName,
		"",
		undeadProcessor
	);

	return character;
}

std::shared_ptr<CMyPlayer> CObjectFactory::CreateMyPlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CMyPlayer>();
	CreateUndeadCharacter(player, heapManager);

	// 싱글 모드에서만 CMovementComponent를 추가한다.
	if (g_is_single) {
		player->SetComponent(std::make_shared<CMovementComponent>());
	}

	// 다우징 로드 component 추가
	auto itemFinder = std::make_shared<CItemFinder>();
	player->SetComponent(itemFinder);

	// Inventory 추가
	std::shared_ptr<CInventory> inventory = std::make_shared<CInventory>(player);
	player->SetInventory(inventory);

	// 테스트! (꼭 지울 것)
	ItemFactory::LoadFromJson("Data/items.json");
	//inventory->AddItem(ItemFactory::Create(100));
	//inventory->AddItem(ItemFactory::Create(1000));

	return player;
}

std::shared_ptr<CPlayer> CObjectFactory::CreatePlayer(CDescriptorHeapManager* heapManager)
{
	auto player = std::make_shared<CPlayer>();
	CreateUndeadCharacter(player, heapManager);
	return player;
}

std::shared_ptr<CMonster> CObjectFactory::CreateMonster(CDescriptorHeapManager* heapManager, MON_TYPE monType, SCENE_TYPE sceneType)
{
	std::shared_ptr<CMonster>     monster;
	std::shared_ptr<CAIComponent> AIComp;

	// AI 생성
	if (g_is_single)
		AIComp = std::make_shared<CAIComponent>();

	switch (monType)
	{
	case MON_TYPE::HUMAN_MONSTER:
	{
		monster = std::make_shared<CHumanMonster>();
		CreateUndeadCharacter(monster, heapManager);
	}
	break;
	case MON_TYPE::ANIMAL_MONSTER:
	{

	}
		break;
	case MON_TYPE::GHOST:
	{

	}
		break;
	default:
		return nullptr;
		break;
	}

	// monster가 속한 scene
	monster->SetCurrentSceneType(sceneType);

	// monster ID, AI, Movement 셋팅 (싱글 모드일 때만)
	// ID 값은 멀티 모드일 때도 여전히 필요하지만 다른 곳에서 셋팅된다. 
	if (g_is_single) {

		// ID 설정
		monster->SetID(s_monster_id_generator);
		++s_monster_id_generator;

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
		monster->SetComponent(std::make_shared<CMovementComponent>());
	}

	return monster;
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
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}