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
#include "MapUtils.h"


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