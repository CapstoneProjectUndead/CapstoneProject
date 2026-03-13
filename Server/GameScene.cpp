#include "pch.h"
// Server쪽 GameScene
#include "GameScene.h"
#include "Collider.h"


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

	std::vector<std::shared_ptr<CObject>> objects;
	//std::vector<MapGenerator::InstanceData> instData = MapGenerator::Generate3DMap();
	//for (const auto& inst : instData) {
	//	for (const std::string& typeName : GameSceneTypeToString(inst.type)) {
	//		std::string meshName = PickRandom(typeName);
	//		if (meshName.empty()) continue;
	//
	//		auto proto = prototypes[meshName];
	//		auto collider = proto->GetComponent<CColliderComponent>();
	//
	//		// 위치/크기 정보를 행렬로 변환하여 추가
	//		auto obj = std::make_shared<CObject>(OBJECT_TYPE::STATIC_OBJECT);
	//
	//		XMMATRIX world = XMLoadFloat4x4(&proto->world_matrix) * XMMatrixRotationY(XMConvertToRadians(inst.rotationY)) * XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);
	//		XMStoreFloat4x4(&obj->world_matrix, world);
	//
	//		// 인스턴스 렌더러에 위치와 리소스 정보 등록
	//		CInstRenderer::GetInstance().AddInstance(
	//			meshComp->GetMesh().get(),
	//			matComp,
	//			obj->world_matrix
	//		);
	//		obj->SetShdaer("inst");
	//
	//		// collider copy(잔디, 돌은 필요X)
	//		std::string base{ typeName };
	//		std::erase_if(base, ::isdigit);
	//		// 길이면 boxShape으로 새로 생성
	//		if ((base != "grass" && base != "stone" && collider)) {
	//			auto copyCollider = std::make_shared<CColliderComponent>(*collider);
	//			obj->SetComponent(copyCollider);
	//			CPhysicsManager::GetInstance().SetCollider(copyCollider);
	//		}
	//		objects.push_back(obj);
	//	}
	//}
	//CInstRenderer::GetInstance().Initialize(GET_DEVICE, GET_CMD_LIST, instData.size());
	//return objects;
}
