#include "stdafx.h"
#include "ObjectFactory.h"
#include "ItemFactory.h"
#include "WorldItem.h"
#include "WorldWeapon.h"
#include "WorldTool.h"
#include "WorldConsumable.h"
#include "WorldOther.h"
#include "WorldTreasure.h"
#include "MineableObject.h"

// Component
#include "MeshComponent.inl"
#include "Collider.h"
#include "Mesh.h"
#include "Animator.h"
#include "Movement.h"

#include "Shader.h"

#include "PhysicsManager.h"
#include "GameFramework.h"
#include "MyPlayer.h"
#include "HumanMonster.h"
#include "Ghost.h"
#include "DogMonster.h"
#include "AIComponent.h"
#include "AIStates.h"
#include "ItemFinder.h"
#include "MapUtils.h"
#include "Inventory.h"
#include "QuickSlot.h"
#include "SceneManager.h" // CSceneManager 사용을 위해 포함 확인 필요

uint32 CObjectFactory::s_monster_id_generator = 1001;

std::shared_ptr<CMaterial> CObjectFactory::GetMaterial(const std::string& name, const EShaderName shaderName)
{
	// SceneManager를 통해 현재 사용하고자 하는 셰이더의 HeapManager를 직접 획득
	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[shaderName]->GetHeapManager();

	std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
	std::shared_ptr<CMaterial> mat = matManager.GetMaterial(name, tex, heapManager);

	return mat;
}

// mesh/material component set
void CObjectFactory::InitStaticComponents(std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, const EShaderName shaderName)
{
	if (!node || node->mesh.positions.empty()) return;

	float radius = XMVectorGetX(XMVector3Length(XMLoadFloat3(&node->mesh.bounds.Extents))) * 1.5f;
	obj->SetBoundingSphere(node->mesh.bounds.Center, radius);

	// MeshRendererComponent
	auto meshRenderer = std::make_shared<CMeshRendererComponent>();
	meshRenderer->SetShader(shaderName);
	obj->SetComponent(meshRenderer);

	// MeshComponent
	auto meshComp = std::make_shared<CMeshComponent>();
	meshComp->SetMeshFromFile<CSkinnedVertex>(GET_DEVICE, GET_CMD_LIST, node);
	obj->world_matrix = node->local_matrix;

	// MaterialComponent
	for (UINT i = 0; i < node->mesh.materials.size(); ++i) {
		auto& material = node->mesh.materials[i];
		auto matComp = std::make_shared<CMaterialComponent>();
		const std::string& texName = material.albedoMap;
		std::shared_ptr<CMaterial> mat;
		if (!texName.empty()) {
			mat = GetMaterial(texName, shaderName); // 내부에서 shaderName 기반으로 처리
			mat->material.albedo = material.albedoColor;
			mat->material.glossiness = material.glossiness;
			matComp->SetMaterial(mat);
		}
		if (!material.normalMap.empty() && mat) {
			auto shaders = CSceneManager::GetInstance().GetShaders();
			CDescriptorHeapManager* heapManager = shaders[shaderName]->GetHeapManager();
			std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, material.normalMap);
			mat->SetNormalIndex(tex);
		}
		meshRenderer->SetRenderUnit(meshComp, matComp, i);
	}

	obj->name = node->name;
}

void CObjectFactory::ProcessNode(std::shared_ptr<CCharacter> character, const std::unique_ptr<CGeometryLoader::FrameNode>& node, std::shared_ptr<CMeshRendererComponent> renderer,
	std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor, const CharacterAnimSet& aniSet, bool isPlayer,
	EColLayer colMask)
{
	// Collider 설정
	EColLayer category = isPlayer ? EColLayer::PLAYER : EColLayer::CHARACTER;
	AddCollider(character, node, category, colMask);

	if (node->mesh.positions.empty()) return;
	character->world_matrix = node->local_matrix;

	// MeshComponent 생성 및 설정
	auto meshComp = std::make_shared<CMeshComponent>();
	meshComp->SetMeshFromFile<CSkinnedVertex>(GET_DEVICE, GET_CMD_LIST, node);

	// 파츠 처리 (머티리얼 등)
	if (partProcessor) {
		partProcessor(node.get(), meshComp, renderer);
	}
}

void CObjectFactory::InitCharacterComponents(std::shared_ptr<CCharacter> character, const std::string& modelFileName,
	std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor,
	CharacterAnimSet aniSet, bool isPlayer, EColLayer colMask)
{
	auto frameRoot = CGeometryLoader::LoadGeometry(modelFileName);
	if (!frameRoot) return;

	// Renderer 및 기초 컴포넌트 설정
	auto renderer = std::make_shared<CMeshRendererComponent>();
	renderer->SetShader(EShaderName::Skinning);
	character->SetComponent(renderer);

	// 메쉬 노드 순회 및 파츠 처리
	ProcessNode(character, frameRoot, renderer, partProcessor, aniSet, isPlayer, colMask);
	// 자식 노드 순회 처리
	for (const auto& child : frameRoot->childrens) {
		ProcessNode(character, child, renderer, partProcessor, aniSet, isPlayer, colMask);
	}

	// 애니메이터 설정
	if (!aniSet.idle.empty()) {
		auto animator = std::make_shared<CAnimatorComponent>();
		character->SetComponent(animator);
		animator->Init(aniSet);
	}

	character->Initialize();
}

void CObjectFactory::AddCollider(std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EColLayer category, EColLayer mask)
{
	if (!node) return;

	// Mesh Collider
	if (!node->mesh_colliders.empty()) {
		for (const auto& mc : node->mesh_colliders) {
			std::unique_ptr<CColliderShape> shape = std::make_unique<CConvexMeshShape>(mc.positions);
			auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
			collider->SetFillter({ category, mask });
			obj->SetComponent(collider);
			if (g_is_single)
				CPhysicsManager::GetInstance().SetCollider(collider);
		}
	}

	// Box Colliders
	for (const auto& box : node->box_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CBoxShape>(box.size, box.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		if (g_is_single)
			CPhysicsManager::GetInstance().SetCollider(collider);
	}

	// Sphere Colliders
	for (const auto& sphere : node->sphere_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CSphereShape>(sphere.radius, sphere.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		if (g_is_single)
			CPhysicsManager::GetInstance().SetCollider(collider);
	}

	// Capsule Colliders
	for (const auto& cap : node->capsule_colliders) {
		std::unique_ptr<CColliderShape> shape = std::make_unique<CCapsuleShape>(cap.radius, cap.height, cap.direction, cap.center);
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		collider->SetFillter({ category, mask });
		obj->SetComponent(collider);
		if (g_is_single)
			CPhysicsManager::GetInstance().SetCollider(collider);
	}
}

void CObjectFactory::SetComponent(std::shared_ptr<CPlayer>& player)
{
	player->SetComponent(std::make_shared<CMovementComponent>());
}

void CObjectFactory::LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EShaderName shaderName)
{
	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

	InitStaticComponents(obj, node, shaderName);

	// 싱글일 때만 collider 생성(멀티면 서버에서 생성)
	if (g_is_single) {
		bool isRoad = (node->name == "park_road" || node->name == "village_road" || node->name == "park_green" || node->name == "house_place");
		bool isMonsterPassable = (node->name == "streetlamp");

		if (isRoad) {
			AddCollider(obj, node, EColLayer::GROUND, EColLayer::ALL_MOB);
		}
		else if (isMonsterPassable) {
			AddCollider(obj, node, EColLayer::OBJECT, EColLayer::PLAYER);
		}
		else {
			AddCollider(obj, node, EColLayer::OBJECT, EColLayer::ALL_MOB);
		}
	}

	obj->Initialize();
	objects.emplace(node->name, std::move(obj));
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateLobby()
{
	std::vector<std::shared_ptr<CObject>> objects;
	std::string fileName{ "../Modeling/lobby_0305.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	for (const auto& children : frameRoot->childrens) {
		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
		InitStaticComponents(obj, children, EShaderName::Skinning);

		// Lobby 특화 Collider 로직
		float radius = XMVectorGetX(XMVector3Length(XMLoadFloat3(&children->mesh.bounds.Extents))) * 1.5f;
		obj->SetBoundingSphere(children->mesh.bounds.Center, radius);

		if (g_is_single) {
			switch (stringToLobbyMeshName(children->name)) {
			case LobbyMeshName::Wall:
			{
				if (!children->mesh_colliders.empty()) {
					std::unique_ptr<CColliderShape> shape = std::make_unique<CConcaveMeshShape>(children->mesh_colliders[0].positions, children->mesh_colliders[0].indices);
					auto collider = std::make_shared<CColliderComponent>(shape, children->mesh.bounds);
					collider->SetFillter({ EColLayer::WALL, EColLayer::ALL_MOB });
					obj->SetComponent(collider);
					CPhysicsManager::GetInstance().SetCollider(collider);
				}
			}
			break;
			default:
				AddCollider(obj, children, EColLayer::OBJECT, EColLayer::ALL_MOB);
				break;
			}
		}
		obj->Initialize();
		objects.push_back(std::move(obj));
	}
	return objects;
}

void CObjectFactory::CopyFromPrototype(std::shared_ptr<CObject> obj, const std::string& name, const XMFLOAT3& position, float rotationY, bool copyMesh)
{
	auto it = prototypes.find(name);
	if (it == prototypes.end()) {
		return;
	}

	auto proto = it->second;
	obj->name = name; // 디버깅용 이름 복사

	// 컬링을 위한 sphere
	obj->SetBoundingSphere(proto->GetBoundingSphere());

	// Transform 계산
	XMMATRIX world = XMLoadFloat4x4(&proto->world_matrix) * XMMatrixRotationY(XMConvertToRadians(rotationY)) * XMMatrixTranslation(position.x, position.y, position.z);
	XMStoreFloat4x4(&obj->world_matrix, world);

	// Renderer에 Mesh/Material 설정 (땅속 보물 등 안 보이는 객체는 스킵)
	if (copyMesh) {
		for (CMeshRendererComponent* renderer : proto->GetComponents<CMeshRendererComponent>()) {
			auto meshRenderer = std::make_shared<CMeshRendererComponent>();
			meshRenderer->SetShader(renderer->GetShader());
			for (const RenderUnit originUnit : renderer->GetRenderUnits()) {
				meshRenderer->SetRenderUnit(originUnit);
			}
			obj->SetComponent(meshRenderer);
		}
	}

	obj->Initialize();
}

void CObjectFactory::LoadGameScene()
{
	CMapAssetManager::GetInstance().initialize();

	if (!prototypes.empty()) return;
	{
		std::string fileName{ "../Modeling/all_map.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/all_map_2.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/map_all_3.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			if (children->name == "tree_1" || children->name == "tree_2")
				LoadFrameNode(prototypes, children, EShaderName::TwoSide);
			else
				LoadFrameNode(prototypes, children);
		}
	}
	{
		std::string fileName{ "../Modeling/obj_treasure.bin" };
		auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

		LoadFrameNode(prototypes, frameRoot);
		for (const auto& children : frameRoot->childrens) {
			LoadFrameNode(prototypes, children);
		}
	}
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateGameScene()
{
	if (prototypes.empty()) LoadGameScene();

	std::vector<std::shared_ptr<CObject>> objects;
	std::vector<MapGenerator::InstanceData> instData = MapGenerator::Generate3DMap();

	// 맵 데이터를 순회하며 보물 좌표 + ID 부여, 몬스터 스폰 위치 추출
	treasures.clear();
	humanMonster_spawn_positions.clear();
	ghost_spawn_positions.clear();
	dog_spawn_positions.clear();

	uint32 treasure_id = 0;

	for (const auto& inst : instData) {

		if (inst.type == MapGenerator::EModelType::TREASURE
			|| inst.type == MapGenerator::EModelType::TREASURE_HIDDEN) {
			treasures.push_back(TreasureInfo{ treasure_id++, inst.position });
		}
		else if (inst.type == MapGenerator::EModelType::MONSTER_HUMAN) {
			humanMonster_spawn_positions.push_back(inst.position);
		}
		else if (inst.type == MapGenerator::EModelType::MONSTER_GHOST) {
			ghost_spawn_positions.push_back(inst.position);
		}
		else if (inst.type == MapGenerator::EModelType::MONSTER_DOG) {
			dog_spawn_positions.push_back(inst.position);
		}
	}

	for (const auto& inst : instData) {
		std::vector<std::string> meshNames = CMapAssetManager::GetInstance().GetMeshNames(inst.type);
		for (const std::string& name : meshNames) {
			if (!prototypes.contains(name)) continue;

			auto proto = prototypes[name];

			bool isTreasure = (inst.type == MapGenerator::EModelType::TREASURE
				|| inst.type == MapGenerator::EModelType::TREASURE_HIDDEN);
			bool isHidden = (inst.type == MapGenerator::EModelType::TREASURE_HIDDEN);

			// 보물이면 CMineableObject 생성 (땅속 보물은 NONE_VISIBLE)
			std::shared_ptr<CObject> obj;
			if (isTreasure) {
				auto mineableType = isHidden ? MINEABLEOBJECT_TYPE::NONE_VISIBLE : MINEABLEOBJECT_TYPE::VISIBLE;
				obj = std::make_shared<CMineableObject>(mineableType);
			}
			else
				obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

			// 땅속 보물은 메시 컴포넌트 스킵 (안 보이게)
			CopyFromPrototype(obj, name, inst.position, inst.rotationY, !isHidden);

			// collider copy (땅속 보물은 콜라이더도 스킵 - 플레이어가 위로 지나갈 수 있게)
			if (!isHidden) {
				for(auto protoCollider : proto->GetComponents<CColliderComponent>()) {
					auto copyCollider = std::make_shared<CColliderComponent>(*protoCollider);
					obj->SetComponent(copyCollider);
					if (isTreasure)
						copyCollider->SetFillter({ copyCollider->GetCollisionFilter().category, EColLayer::PLAYER});
					CPhysicsManager::GetInstance().SetCollider(copyCollider);
				}
			}

			obj->Initialize();
			objects.push_back(obj);
		}
	}
	return objects;
}

std::vector<std::shared_ptr<CObject>> CObjectFactory::CreateGameSceneByServer(const std::vector<MapGenerator::InstanceData>& instanceData)
{
	if (prototypes.empty()) LoadGameScene();

	treasures.clear();
	humanMonster_spawn_positions.clear();
	ghost_spawn_positions.clear();
	dog_spawn_positions.clear();

	std::vector<std::shared_ptr<CObject>> objects;

	for (const auto& inst : instanceData) {
		std::vector<std::string> meshNames = CMapAssetManager::GetInstance().GetMeshNames(inst.type, inst.model);
		for (const std::string& name : meshNames) {
			auto proto = prototypes[name];

			bool isTreasure = (inst.type == MapGenerator::EModelType::TREASURE
				|| inst.type == MapGenerator::EModelType::TREASURE_HIDDEN);
			bool isHidden = (inst.type == MapGenerator::EModelType::TREASURE_HIDDEN);

			// 보물이면 CMineableObject 생성 (땅속 보물은 NONE_VISIBLE)
			std::shared_ptr<CObject> obj;
			if (isTreasure) {
				auto mineableType = isHidden ? MINEABLEOBJECT_TYPE::NONE_VISIBLE : MINEABLEOBJECT_TYPE::VISIBLE;
				obj = std::make_shared<CMineableObject>(mineableType);
			}
			else
				obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

			// 땅속 보물은 메시 컴포넌트 스킵 (안 보이게)
			CopyFromPrototype(obj, name, inst.position, inst.rotationY, !isHidden);
			obj->Initialize();
			objects.push_back(obj);
		}
	}
	return objects;
}

void CObjectFactory::CreateUndeadCharacter(std::shared_ptr<CPlayer> character)
{
	std::string fileName{ "../Modeling/undead_char.bin" };

	// 기본적으로 캐릭터용 스킨드 셰이더 힙매니저 획득
	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[EShaderName::Skinning]->GetHeapManager();

	// material 미리 Load
	std::vector<std::string> resourceNames = {
		"body_ganga", "body_nyao", "body_toto",
		"eartail",
		"eyes_ganga", "eyes_nyao", "eyes_toto",
		"mouse_ganga", "mouse_nyao", "mouse_toto"
	};
	for (const std::string& name : resourceNames) {
		std::shared_ptr<CTexture> tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, name);
		matManager.LoadMaterial(name, tex, heapManager);
	}

	auto undeadProcessor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {

			// 머티리얼 생성 및 렌더 유닛 등록 후 material return
			auto CreateUnit = [&](const std::string& texName) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex, heapManager);
				matComp->SetMaterial(mat);

				RenderUnit unit{ meshComp, matComp, 0 };
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
			case UndeadMeshName::Unknown:
				for (auto& material : node->mesh.materials) {
					CreateUnit(material.albedoMap);
				}
				break;
			}
		};

	InitCharacterComponents(
		character,
		fileName,
		undeadProcessor,
		{ "Ganga_idle", "Ganga_walk", "Ganga_run", "Ganga_expect" },
		true,
		static_cast<EColLayer>(EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND | EColLayer::CHARACTER)
	);
}

void CObjectFactory::CreateHumanCharacter(std::shared_ptr<CCharacter> character)
{
	std::string fileName{ "../Modeling/Human_monster.bin" };

	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[EShaderName::Skinning]->GetHeapManager();

	auto Processor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
			// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
			auto CreateUnit = [&](const std::string& texName, UINT submeshIndex) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex, heapManager);
				matComp->SetMaterial(mat);
				RenderUnit unit{ meshComp, matComp, submeshIndex };
				renderer->SetRenderUnit(unit);
				};

			for (UINT i = 0; i < node->mesh.materials.size(); ++i) {
				auto& material = node->mesh.materials[i];
				CreateUnit(material.albedoMap, i);
			}
		};

	InitCharacterComponents(
		character,
		fileName,
		Processor,
		{ "Human_monster_idle", "Human_monster_walk", "Human_monster_run", "Human_monster_attack" },
		false,
		static_cast<EColLayer>(EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND | EColLayer::PLAYER)
	);
}

void CObjectFactory::CreateDogCharacter(std::shared_ptr<CCharacter> character)
{
	std::string fileName{ "../Modeling/Dog.bin" };

	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[EShaderName::Skinning]->GetHeapManager();

	auto Processor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
			// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
			auto CreateUnit = [&](const std::string& texName, UINT submeshIndex) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex, heapManager);
				matComp->SetMaterial(mat);
				RenderUnit unit{ meshComp, matComp, submeshIndex };
				renderer->SetRenderUnit(unit);
				};

			for (UINT i = 0; i < node->mesh.materials.size(); ++i) {
				auto& material = node->mesh.materials[i];
				CreateUnit(material.albedoMap, i);
			}
		};

	InitCharacterComponents(
		character,
		fileName,
		Processor,
		{ "idle", "walk", "run", "bite" },
		false,
		static_cast<EColLayer>(EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND | EColLayer::PLAYER)
	);
}

void CObjectFactory::CreateGhostCharacter(std::shared_ptr<CCharacter> character)
{
	std::string fileName{ "../Modeling/Ghost3.bin" };

	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[EShaderName::Skinning]->GetHeapManager();

	auto Processor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
			// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
			auto CreateUnit = [&](const std::string& texName, UINT submeshIndex) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex, heapManager);
				matComp->SetMaterial(mat);
				RenderUnit unit{ meshComp, matComp, submeshIndex };
				renderer->SetRenderUnit(unit);
				};

			for (UINT i = 0; i < node->mesh.materials.size(); ++i) {
				auto& material = node->mesh.materials[i];
				CreateUnit(material.albedoMap, i);
			}
		};

	InitCharacterComponents(
		character,
		fileName,
		Processor,
		{ "Ghost_idle", "Ghost_walk", "Ghost_run", "Ghost_attack" },
		false,
		static_cast<EColLayer>(EColLayer::WALL | EColLayer::GROUND)
	);
}

std::shared_ptr<CCharacter> CObjectFactory::CreateReaper()
{
	std::shared_ptr<CCharacter> character = std::make_shared<CCharacter>(OBJECT_TYPE::STATIC_OBJECT);
	std::string fileName{ "../Modeling/Reaper.bin" };

	auto shaders = CSceneManager::GetInstance().GetShaders();
	CDescriptorHeapManager* heapManager = shaders[EShaderName::Skinning]->GetHeapManager();

	auto undeadProcessor = [&](const CGeometryLoader::FrameNode* node, std::shared_ptr<CMeshComponent> meshComp,
		std::shared_ptr<CMeshRendererComponent> renderer) {
			// 머티리얼 생성 및 렌더 유닛 등록 헬퍼
			auto CreateUnit = [&](const std::string& texName, UINT submeshIndex) {
				auto matComp = std::make_shared<CMaterialComponent>();
				auto tex = texManager.GetTexture(GET_DEVICE, GET_CMD_LIST, heapManager, texName);
				auto mat = matManager.GetMaterial(texName, tex, heapManager);
				matComp->SetMaterial(mat);
				RenderUnit unit{ meshComp, matComp, submeshIndex };
				renderer->SetRenderUnit(unit);
				};

			for (UINT i = 0; i < node->mesh.materials.size(); ++i) {
				auto& material = node->mesh.materials[i];
				CreateUnit(material.albedoMap, i);
			}
			renderer->SetShader(EShaderName::Skinning);
		};

	InitCharacterComponents(
		character,
		fileName,
		undeadProcessor,
		{},
		false
	);

	return character;
}

std::shared_ptr<CMyPlayer> CObjectFactory::CreateMyPlayer()
{
	auto player = std::make_shared<CMyPlayer>();
	CreateUndeadCharacter(player);

	// 싱글 모드에서만 CMovementComponent를 추가한다.
	if (g_is_single) {
		player->SetComponent(std::make_shared<CMovementComponent>());
	}

	// 다우징 로드 component 추가
	auto itemFinder = std::make_shared<CItemFinder>();
	player->SetComponent(itemFinder);
	itemFinder->SetEnable(false);

	// Inventory 추가
	std::shared_ptr<CInventory> inventory = std::make_shared<CInventory>(player);
	player->SetInventory(inventory);

	// QuickSlot 추가
	auto quickSlot = std::make_shared<CQuickSlot>();
	inventory->SetQuickSlot(quickSlot);
	player->SetQuickSlot(quickSlot);

	// 약한 참조
	quickSlot->SetOwner(player);

	return player;
}

std::shared_ptr<CPlayer> CObjectFactory::CreatePlayer()
{
	auto player = std::make_shared<CPlayer>();
	CreateUndeadCharacter(player);
	return player;
}

std::shared_ptr<CMonster> CObjectFactory::CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType)
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
		monster->SetCurrentSceneType(sceneType);
		CreateHumanCharacter(monster);
	}
	break;
	case MON_TYPE::ANIMAL_MONSTER:
	{
		monster = std::make_shared<CDogMonster>();
		monster->SetCurrentSceneType(sceneType);
		CreateDogCharacter(monster);
	}
	break;
	case MON_TYPE::GHOST:
	{
		monster = std::make_shared<CGhost>();
		monster->SetCurrentSceneType(sceneType);
		CreateGhostCharacter(monster);
	}
	break;
	default:
		return nullptr;
		break;
	}

	// monster ID, AI, Movement 셋팅 (싱글 모드일 때만)
	if (g_is_single) {
		// ID 설정
		monster->SetID(s_monster_id_generator);
		++s_monster_id_generator;

		// IDLE, PATROL, TRACE, ATTACK 상태는 모든 몬스터가 공통으로 가진다.
		AIComp->AddState(std::make_shared<CIdleState>());
		AIComp->AddState(std::make_shared<CPatrolState>());
		AIComp->AddState(std::make_shared<CTraceState>());
		AIComp->AddState(std::make_shared<CAttackState>());
		AIComp->AddState(std::make_shared<CFleeState>());

		// 항상 IDLE 로 시작.
		AIComp->SetState(AI_STATE::MONSTER_IDLE);

		// AI 컴포넌트 Monster에 등록
		monster->SetComponent(AIComp);

		// Movement 컴포넌트 추가. (순서가 중요. AI -> Movement 순서로 가야함.)
		monster->SetComponent(std::make_shared<CMovementComponent>());
	}

	return monster;
}

std::shared_ptr<CWorldItem> CObjectFactory::CreateWorldItem(uint16 itemID)
{
	auto item = ItemFactory::Create(itemID);
	if (!item)
		return nullptr;

	std::shared_ptr<CWorldItem> worldItem;

	switch (item->GetItemType())
	{
	case ITEM_TYPE::EQUIPMENT:
		if (item->GetSubType() >= ITEM_SUB_TYPE::MELEE_WEAPON)
			worldItem = std::make_shared<CWorldWeapon>(item);
		else
			worldItem = std::make_shared<CWorldTool>(item);
		break;
	case ITEM_TYPE::CONSUMABLE:
		worldItem = std::make_shared<CWorldConsumable>(item);
		break;
	case ITEM_TYPE::ETC:
		worldItem = std::make_shared<CWorldOther>(item);
		break;
	case ITEM_TYPE::TREASURE:
		worldItem = std::make_shared<CWorldTreasure>(item);
		break;
	default:
		return nullptr;
	}

	// 렌더링 컴포넌트 설정
	std::string modelName = ItemFactory::GetModelName(item->GetItemId());
	CopyFromPrototype(worldItem, modelName, {}, {});

	return worldItem;
}

void CObjectFactory::LoadNode(const std::string fileName, EShaderName shaderName)
{
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);
	if (!frameRoot) return;

	LoadFrameNode(prototypes, frameRoot, shaderName);
	for (const auto& children : frameRoot->childrens) {
		LoadFrameNode(prototypes, children, shaderName);
	}
}

void CObjectFactory::LoadItemFrame()
{
	{
		std::string fileName{ "../Modeling/Equip.bin" };
		LoadNode(fileName);
	}
	{
		std::string fileName{ "../Modeling/Food.bin" };
		LoadNode(fileName);
	}
	{
		std::string fileName{ "../Modeling/dowsing_rod_model.bin" };
		LoadNode(fileName);
	}
	{
		std::string fileName{ "../Modeling/treasure.bin" };
		LoadNode(fileName);
	}
}

void CObjectFactory::LoadTwoSideFrame()
{
	{
		std::string fileName{ "../Modeling/flapper.bin" };
		LoadNode(fileName, EShaderName::TwoSide);
	}
}

// string to enum mapping
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
	};

	auto it = table.find(str);
	return (it != table.end()) ? it->second : LobbyMeshName::Unknown;
}