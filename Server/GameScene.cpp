#include "pch.h"
// Server쪽 GameScene
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
	if (node->mesh.positions.empty()) 
		return;

	auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);

	auto SetColliderComp = [&node, obj](std::unique_ptr<CColliderShape>& shape) {
		auto collider = std::make_shared<CColliderComponent>(shape, node->mesh.bounds);
		CollisionFilter filter;
		filter.category = EColLayer::OBJECT;
		filter.mask = EColLayer::PLAYER;
		collider->SetFillter(filter);
		collider->owner = obj.get();
		collider->Update(0.0f);
		obj->SetComponent(collider);
		};
	// ColliderComponent 생성
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
		boxCollider->owner = obj.get();
		boxCollider->Update(0.0f);
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

	instance_data = MapGenerator::Generate3DMap();

	for (const auto& inst : instance_data) {

		for (const std::string& typeName : GameSceneTypeToString(inst.type)) {

			std::string meshName = PickRandom(typeName);
			if (meshName.empty()) continue;
		
			auto proto = prototypes[meshName];
			auto collider = proto->GetComponent<CColliderComponent>();
		
			// 위치/크기 정보를 행렬로 변환하여 추가
			auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
			
			XMMATRIX world = XMLoadFloat4x4(&proto->GetWorldMatrix()) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
			XMStoreFloat4x4(&obj->GetWorldMatrix(), world);
			
			// collider copy(잔디, 돌은 필요X)
			std::string base{ typeName };
			std::erase_if(base, ::isdigit);
			// 길이면 boxShape으로 새로 생성
			if ((base != "grass" && base != "stone" && collider)) {
				auto copyCollider = std::make_shared<CColliderComponent>(*collider);
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

		{ MapGenerator::EModelType::WALL,					{"house_place"} },// 임시

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

// grass, stone 등 랜덤 카테고리 선택
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

	// key가 카테고리인지 확인
	auto it = categoryTable.find(key);
	if (it != categoryTable.end()) {
		const auto& list = it->second;
		int idx = rand() % list.size();
		return list[idx];
	}

	return key;
}
