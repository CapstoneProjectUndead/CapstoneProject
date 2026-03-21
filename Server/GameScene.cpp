#include "pch.h"
// Server�� GameScene
#include "GameScene.h"
#include "Player.h"
#include "User.h"
#include "Room.h"
#include "Collider.h"
#include "PhysicsManager.h"
#include "GeometryLoader.h"
#include "HumanMonster.h"
#include "ServerObjectFactory.h"


CGameScene::CGameScene(uint32 roomId)
	: CScene(SCENE_TYPE::GAME)
{

}

CGameScene::~CGameScene()
{

}

void CGameScene::Start()
{
	
}

void CGameScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}
 
void CGameScene::LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node)
{
	if (node->mesh.positions.empty()) return;

	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
	obj->GetWorldMatrix() = node->localMatrix;

	auto SetColliderComp = [&node, obj](std::unique_ptr<CColliderShape>& shape) {
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;
		collider->SetFillter(filter);
		obj->SetComponent(collider);
		};
	// ColliderComponent ����
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

	objects.emplace(node->name, std::move(obj));
}

void CGameScene::LoadGameScene()
{
	if (!prototypes.empty()) 
		return;

	std::string fileName{ "../Modeling/all_map.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);

	LoadFrameNode(prototypes, frameRoot);
	for (const auto& children : frameRoot->childrens) {
		LoadFrameNode(prototypes, children);
	}
}

void CGameScene::CreateGameScene()
{
	if (prototypes.empty()) 
		LoadGameScene();

	vector<MapGenerator::InstanceData> instanceData = MapGenerator::Generate3DMap();

	// 맵 데이터를 순회하며 보물 좌표만 빼오기
	for (const auto& inst : instanceData) {
		if (inst.type == MapGenerator::EModelType::TREASURE) {
			treasures.push_back(TreasureInfo{ inst.position });
		}
	}

	for (auto& inst : instanceData) {
		for (const std::string& typeName : GameSceneTypeToString(inst.type)) {

			EModelVariant model = PickRandomVariant(typeName);
			if (model == EModelVariant::NONE) continue;

			std::string meshName = GetVariantFileName(model);
			if (meshName.empty()) continue;

			// 오브젝트별 렌더링 데이터를 별도 벡터에 저장 (셀당 여러 오브젝트 지원)
			MapGenerator::InstanceData rd;
			rd.position = inst.position;
			rd.rotationY = inst.rotationY;
			rd.type = inst.type;
			rd.model = model;
			map_instance_data.push_back(rd);

			auto proto = prototypes[meshName];
			auto collider = proto->GetComponent<CColliderComponent>();

			auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

			XMMATRIX world = XMLoadFloat4x4(&proto->GetWorldMatrix()) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
			XMStoreFloat4x4(&obj->GetWorldMatrix(), world);

			// collider copy
			std::string base{ typeName };
			std::erase_if(base, ::isdigit);

			// boxShape
			if ((base != "grass" && base != "stone" && collider)) {
				auto copyCollider = std::make_shared<CColliderComponent>(*collider);
				copyCollider->owner = obj.get();
				copyCollider->Update(0.0f);
				obj->SetComponent(copyCollider);
				CPhysicsManager::GetInstance().SetCollider(copyCollider);
			}
			static_objects.push_back(obj);
		}
	}
}

std::vector<std::string> CGameScene::GameSceneTypeToString(const MapGenerator::EModelType& type)
{
	static const std::unordered_map<MapGenerator::EModelType, std::vector<std::string>> table = {
		{ MapGenerator::EModelType::ROAD,					{"park_road", "stone"} },
		{ MapGenerator::EModelType::PARK_GREEN,				{"park_green", "grass"} },
		{ MapGenerator::EModelType::VILLAGE_ROAD,			{"village_road"} },
		{ MapGenerator::EModelType::HOUSE_INNTER,			{"house_place"} },

		{ MapGenerator::EModelType::WALL,					{"house_place"} },

		{ MapGenerator::EModelType::HOUSE_WALL_CORNER,		{"wall_2001"} },
		{ MapGenerator::EModelType::HOUSE_WALL_STRAIGHT,	{"wall_1002"} },
		{ MapGenerator::EModelType::HOUSE_WALL_EMPTY,		{"wall_1003"} },
		{ MapGenerator::EModelType::DOOR,					{"wall_1_door001"} },
		{ MapGenerator::EModelType::CORNER_DOOR,			{"wall_2_door001"} },

		{ MapGenerator::EModelType::KIOSK,					{"vending_machine001"} },
		{ MapGenerator::EModelType::TREE,					{"tree"} },
		{ MapGenerator::EModelType::TREASURE,				{"trashcan"} },
		{ MapGenerator::EModelType::BENCH,					{"park_bench"} },
		{ MapGenerator::EModelType::SMALL_BUSH,				{"small_bush"} },
		{ MapGenerator::EModelType::SEESAW,					{"seesaw001"} },

		{ MapGenerator::EModelType::UNKNOWN,				{"park_road"} },
	};


	auto it = table.find(type);
	return it->second;
}

std::string CGameScene::GetVariantFileName(EModelVariant variant)
{
	static const std::unordered_map<EModelVariant, std::string> variantToString = {
		{ EModelVariant::NONE, "" },

		// --- [고정 에셋들] ---
		{ EModelVariant::PARK_ROAD, "park_road" },
		{ EModelVariant::PARK_GREEN, "park_green" },
		{ EModelVariant::VILLAGE_ROAD, "village_road" },
		{ EModelVariant::HOUSE_PLACE, "house_place" },
		{ EModelVariant::WALL_1002, "wall_1002" },
		{ EModelVariant::WALL_2001, "wall_2001" },
		{ EModelVariant::WALL_1003, "wall_1003" },
		{ EModelVariant::WALL_1_DOOR001, "wall_1_door001" },
		{ EModelVariant::WALL_2_DOOR001, "wall_2_door001" },
		{ EModelVariant::VENDING_MACHINE_001, "vending_machine001" },
		{ EModelVariant::SEESAW_001, "seesaw001" },

		// Grass
		{ EModelVariant::GRASS_019, "grass019" }, { EModelVariant::GRASS_020, "grass020" },
		{ EModelVariant::GRASS_021, "grass021" }, { EModelVariant::GRASS_022, "grass022" },
		{ EModelVariant::GRASS_023, "grass023" }, { EModelVariant::GRASS_024, "grass024" },
		{ EModelVariant::GRASS_025, "grass025" }, { EModelVariant::GRASS_026, "grass026" },
		{ EModelVariant::GRASS_027, "grass027" }, { EModelVariant::GRASS_028, "grass028" },
		{ EModelVariant::GRASS_029, "grass029" }, { EModelVariant::GRASS_030, "grass030" },
		{ EModelVariant::GRASS_031, "grass031" }, { EModelVariant::GRASS_032, "grass032" },
		{ EModelVariant::GRASS_033, "grass033" }, { EModelVariant::GRASS_034, "grass034" },
		{ EModelVariant::GRASS_035, "grass035" }, { EModelVariant::GRASS_036, "grass036" },
		{ EModelVariant::GRASS_037, "grass037" },

		// Stone
		{ EModelVariant::STONE_011, "stone011" }, { EModelVariant::STONE_012, "stone012" },
		{ EModelVariant::STONE_013, "stone013" }, { EModelVariant::STONE_014, "stone014" },
		{ EModelVariant::STONE_015, "stone015" }, { EModelVariant::STONE_016, "stone016" },
		{ EModelVariant::STONE_017, "stone017" }, { EModelVariant::STONE_018, "stone018" },
		{ EModelVariant::STONE_019, "stone019" }, { EModelVariant::STONE_020, "stone020" },
		{ EModelVariant::STONE_021, "stone021" }, { EModelVariant::STONE_022, "stone022" },
		{ EModelVariant::STONE_023, "stone023" }, { EModelVariant::STONE_024, "stone024" },

		// Props
		{ EModelVariant::PARK_BENCH_002, "park_bench002" },
		{ EModelVariant::PARK_BENCH_003, "park_bench003" },
		{ EModelVariant::SMALL_BUSH_001, "small_bush001" },
		{ EModelVariant::SMALL_BUSH_002, "small_bush002" },
		{ EModelVariant::TREE_002, "tree002" },
		{ EModelVariant::PINETREE, "pinetree" },
		{ EModelVariant::TRASHCAN_001, "trashcan001" },
		{ EModelVariant::TRASHCAN_002, "trashcan002" }
	};

	auto it = variantToString.find(variant);
	return (it != variantToString.end()) ? it->second : "";
}

std::string CGameScene::PickRandom(const std::string& key)
{
	static const std::unordered_map<std::string, std::vector<std::string>> categoryTable = {
		{ "grass", {
			"grass019","grass020","grass021","grass022","grass023","grass024",
			"grass025","grass026","grass027","grass028","grass029","grass030",
			"grass031","grass032","grass033","grass034","grass035","grass036","grass037", ""
		}},
		{ "stone", {
			"stone011","stone012","stone013","stone014","stone015","stone016",
			"stone017","stone018","stone019","stone020","stone021","stone022",
			"stone023","stone024", ""
		}},
		{ "park_bench", {
			"park_bench002","park_bench003"
		}},
		{ "small_bush", {
			"small_bush001","small_bush002"
		}},
		{ "tree", {
			"tree002","pinetree"
		}},
		{ "trashcan", {
			"trashcan001","trashcan002"
		}},
	};

	auto it = categoryTable.find(key);
	if (it != categoryTable.end()) {
		const auto& list = it->second;
		int idx = rand() % list.size();
		return list[idx];
	}

	return key;
}

EModelVariant CGameScene::PickRandomVariant(const std::string& key)
{
	// 1. 랜덤 카테고리인지 먼저 확인 (기존 로직)
	static const std::unordered_map<std::string, std::vector<EModelVariant>> categoryTable = {
		
		{ "grass", {
			EModelVariant::GRASS_019, EModelVariant::GRASS_020, EModelVariant::GRASS_021,
			EModelVariant::GRASS_022, EModelVariant::GRASS_023, EModelVariant::GRASS_024,
			EModelVariant::GRASS_025, EModelVariant::GRASS_026, EModelVariant::GRASS_027,
			EModelVariant::GRASS_028, EModelVariant::GRASS_029, EModelVariant::GRASS_030,
			EModelVariant::GRASS_031, EModelVariant::GRASS_032, EModelVariant::GRASS_033,
			EModelVariant::GRASS_034, EModelVariant::GRASS_035, EModelVariant::GRASS_036,
			EModelVariant::GRASS_037, EModelVariant::NONE
		}},

		{ "stone", {
			EModelVariant::STONE_011, EModelVariant::STONE_012, EModelVariant::STONE_013,
			EModelVariant::STONE_014, EModelVariant::STONE_015, EModelVariant::STONE_016,
			EModelVariant::STONE_017, EModelVariant::STONE_018, EModelVariant::STONE_019,
			EModelVariant::STONE_020, EModelVariant::STONE_021, EModelVariant::STONE_022,
			EModelVariant::STONE_023, EModelVariant::STONE_024, EModelVariant::NONE
		}},

		{ "park_bench", { EModelVariant::PARK_BENCH_002, EModelVariant::PARK_BENCH_003 }},
		{ "small_bush", { EModelVariant::SMALL_BUSH_001, EModelVariant::SMALL_BUSH_002 }},
		{ "tree",       { EModelVariant::TREE_002,       EModelVariant::PINETREE }},
		{ "trashcan",   { EModelVariant::TRASHCAN_001,   EModelVariant::TRASHCAN_002 }},
	};

	auto it = categoryTable.find(key);
	if (it != categoryTable.end()) {
		const auto& list = it->second;
		return list[rand() % list.size()];
	}

	// 2. 랜덤이 아니라면? 고정 메쉬 테이블에서 검색!
	static const std::unordered_map<std::string, EModelVariant> fixedTable = {
		{ "park_road", EModelVariant::PARK_ROAD },
		{ "park_green", EModelVariant::PARK_GREEN },
		{ "village_road", EModelVariant::VILLAGE_ROAD },
		{ "house_place", EModelVariant::HOUSE_PLACE },
		{ "wall_1002", EModelVariant::WALL_1002 },
		{ "wall_2001", EModelVariant::WALL_2001 },
		{ "wall_1003", EModelVariant::WALL_1003 },
		{ "wall_1_door001", EModelVariant::WALL_1_DOOR001 },
		{ "wall_2_door001", EModelVariant::WALL_2_DOOR001 },
		{ "vending_machine001", EModelVariant::VENDING_MACHINE_001 },
		{ "seesaw001", EModelVariant::SEESAW_001 }
	};

	auto fixedIt = fixedTable.find(key);
	if (fixedIt != fixedTable.end()) {
		return fixedIt->second;
	}

	// 여기까지 왔는데도 없으면 진짜 에러거나 빈 공간
	return EModelVariant::NONE;
}
