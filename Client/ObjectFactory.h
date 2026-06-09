#pragma once
#include "Material.h"
#include "Texture.h"

// 맵 생성 알고리즘
#include <MapGenerator/MapGenerator.h>
#include <CollisionAlgorithm.h>

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
	struct MaterialData;
}
enum EColLayer : uint32_t;

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;

public:
	// heapManager 인자 제거, 셰이더 이름을 받아 내부에서 힙 매니저를 찾음
	std::shared_ptr<CMaterial> GetMaterial(const std::string& name, const EShaderName shaderName);

	// init component(공통 컴포넌트 구성 초기화 헬퍼 함수)
	void InitStaticComponents(std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, const EShaderName shaderName);

	void ProcessNode(std::shared_ptr<CCharacter> character, const std::unique_ptr<CGeometryLoader::FrameNode>& node, std::shared_ptr<CMeshRendererComponent> renderer,
		std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor, const CharacterAnimSet& aniSet, bool isPlayer,
		EColLayer colMask = static_cast<EColLayer>(EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND));

	void InitCharacterComponents(std::shared_ptr<CCharacter> character, const std::string& modelFileName,
		std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor,
		CharacterAnimSet aniSet, bool isPlayer,
		EColLayer colMask = static_cast<EColLayer>(EColLayer::WALL | EColLayer::OBJECT | EColLayer::GROUND));

	// concave는 따로 생성 필요
	void AddCollider(std::shared_ptr<CObject> obj, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EColLayer category, EColLayer mask);

	// MovementComp Set
	void SetComponent(std::shared_ptr<CPlayer>& player);

	// Create Map
	void LoadFrameNode(std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node, EShaderName shaderName = EShaderName::Skinning);

	// Lobby
	std::vector<std::shared_ptr<CObject>> CreateLobby();

	// GameScene 모델 파츠 load
	// Collider Component는 별도로 설정 필요
	void CopyFromPrototype(std::shared_ptr<CObject> obj, const std::string& name, const XMFLOAT3& position, float rotationY, bool copyMesh = true);
	void LoadGameScene();
	std::vector<std::shared_ptr<CObject>> CreateGameScene();
	std::vector<std::shared_ptr<CObject>> CreateGameSceneByServer(const std::vector<MapGenerator::InstanceData>& instanceData);

	// Load model
	void CreateUndeadCharacter(std::shared_ptr<CPlayer> character);
	void CreateHumanCharacter(std::shared_ptr<CCharacter> character);
	void CreateGhostCharacter(std::shared_ptr<CCharacter> character);
	void CreateDogCharacter(std::shared_ptr<CCharacter> character);
	std::shared_ptr<CCharacter> CreateReaper();

	// Create character
	std::shared_ptr<CMyPlayer> CreateMyPlayer();
	std::shared_ptr<CPlayer> CreatePlayer();
	std::shared_ptr<CMonster> CreateMonster(MON_TYPE monType, SCENE_TYPE sceneType);

	// WorldItem 생성
	std::shared_ptr<CWorldItem> CreateWorldItem(uint16 itemID);
	// 미리 prototypes에 적재
	void LoadItemFrame();
	// 은면이 보여야하는 오브젝트 prototypes에 적재
	void LoadTwoSideFrame();

	// getter&setter
	std::vector<TreasureInfo>& GetTreauseres() { return treasures; }
	std::vector<XMFLOAT3>&    GetHumanMonsterSpawnPositions() { return humanMonster_spawn_positions; }
	std::vector<XMFLOAT3>&    GetGhostSpawnPositions() { return ghost_spawn_positions; }
	std::vector<XMFLOAT3>&    GetDogMonsterSpawnPositions() { return dog_spawn_positions; }
	std::vector<MapGenerator::InstanceData>& GetInstData() { return inst_data; }

	// 커스터마이징 Update
	void UpdatePlayerTextures(std::shared_ptr<CPlayer> player);

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
		Unknown
	};

	// rawMatData를 적용
	std::shared_ptr<CMaterialComponent> CreateMaterialComponent(const CGeometryLoader::MaterialData& rawMatData, const EShaderName shaderName, CDescriptorHeapManager* heapManager);
	void LoadNode(const std::string fileName, EShaderName shaderName = EShaderName::Skinning);
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
	std::vector<XMFLOAT3>    humanMonster_spawn_positions;
	std::vector<XMFLOAT3>    ghost_spawn_positions;
	std::vector<XMFLOAT3>    dog_spawn_positions;
	std::vector<MapGenerator::InstanceData>	inst_data;
};

