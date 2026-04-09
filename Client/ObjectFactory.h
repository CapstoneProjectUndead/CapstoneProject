#pragma once
#include "Material.h"
#include "Texture.h"

// 맵 생성 알고리즘
#include <MapGenerator/MapGenerator.h>

class CDescriptorHeapManager;
class CCharacter;
class CObject;
class CPlayer;
class CMyPlayer;
class CMonster;
class CHumanMonster;
class CMeshComponent;
class CMeshRendererComponent;
class CWorldItem;
struct CharacterAnimSet;

namespace CGeometryLoader {
	struct FrameNode;
}
enum EColLayer : uint32_t;

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;
public:
	std::shared_ptr<CMaterial> GetMaterial(CDescriptorHeapManager* heapManager, const std::string& name);
	// init component(공통 컴포넌트 구성 초기화 헬퍼 함수)
	void InitStaticComponents(std::shared_ptr<CObject> obj, CDescriptorHeapManager* heapManager, const std::unique_ptr<CGeometryLoader::FrameNode>& node, const std::string& shaderName = "inst");
	void InitCharacterComponents(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager, const std::string& modelFileName, 
		std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor,
		CharacterAnimSet aniSet, bool isPlayer);
	// concave는 따로 생성 필요
	void AddCollider(std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EColLayer category, EColLayer mask);
	// MovementComp Set
	void SetComponent(std::shared_ptr<CPlayer>& player);

	// Create Map
	void LoadFrameNode(CDescriptorHeapManager* heapManager, std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node);
	// Lobby
	std::vector<std::shared_ptr<CObject>> CreateLobby(CDescriptorHeapManager* heapManager);
	// GameScene 모델 파츠 load
	// Collider Component는 별도로 설정 필요
	void CopyFromPrototype(std::shared_ptr<CObject> obj, const std::string& name, const XMFLOAT3& position, float rotationY);
	void LoadGameScene(CDescriptorHeapManager* heapManager);
	std::vector<std::shared_ptr<CObject>> CreateGameScene(CDescriptorHeapManager* heapManager);
	std::vector<std::shared_ptr<CObject>> CreateGameSceneByServer(CDescriptorHeapManager* heapManager, const std::vector<MapGenerator::InstanceData>& instanceData);
	// Load model
	void CreateUndeadCharacter(std::shared_ptr<CPlayer> character, CDescriptorHeapManager* heapManager);
	void CreateHumanCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager);
	void CreateGhostCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager);
	std::shared_ptr<CCharacter> CreateReaper(CDescriptorHeapManager* heapManager);

	// Create character
	std::shared_ptr<CMyPlayer> CreateMyPlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CPlayer> CreatePlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMonster> CreateMonster(CDescriptorHeapManager* heapManager, MON_TYPE monType, SCENE_TYPE sceneType);

	// WorldItem 생성
	std::shared_ptr<CWorldItem> CreateWorldItem(uint16 itemID, CDescriptorHeapManager* heapManager);
	// 미리 prototypes에 적재
	void LoadItemFrame(CDescriptorHeapManager* heapManager);

	// getter&setter
	std::vector<TreasureInfo>& GetTreauseres() { return treasures; }

	// 외부 참조용
	std::shared_ptr<CObject> GetPrototype(const std::string& name) {
		auto it = prototypes.find(name);
		return (it != prototypes.end()) ? it->second : nullptr;
	}
private:
	enum class UndeadMeshName {
		body,
		Bunny_ear,
		Bunny_tail,
		Cat_ear,
		Cat_tail,
		Dog_ear,
		Dog_tail,
		eyes,
		mouse,
		Unknown
	};

	enum class LobbyMeshName {
		Wall,
		Floor,
		GroundPipe,
		Unknown
	};


	// string to enum mapping
	UndeadMeshName stringToUndeadMeshName(const std::string& str);
	LobbyMeshName stringToLobbyMeshName(const std::string& str);
private:
	CMaterialManager matManager;
	CTextureManager texManager;
	std::map<std::string, std::shared_ptr<CObject>> prototypes;	// gameScene, item model 적재(CopyFromPrototype로 복사해서 사용)

	// 싱글모드에서만 의미있다.
	static uint32 s_monster_id_generator;

	std::vector<TreasureInfo> treasures;
};

