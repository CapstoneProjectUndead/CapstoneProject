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
namespace CGeometryLoader {
	struct FrameNode;
}

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;
	std::shared_ptr<CMaterial> GetMaterial(CDescriptorHeapManager* heapManager, const std::string& name);
	void LoadFrameNode(CDescriptorHeapManager* heapManager, std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<CGeometryLoader::FrameNode>& node);
	// MovementComp Set
	void SetComponent(std::shared_ptr<CPlayer>& player);
	// Lobby
	std::vector<std::shared_ptr<CObject>> CreateLobby(CDescriptorHeapManager* heapManager);
	// GameScene 모델 파츠 load
	void LoadGameScene(CDescriptorHeapManager* heapManager);
	std::vector<std::shared_ptr<CObject>> CreateGameScene(CDescriptorHeapManager* heapManager);
	std::vector<std::shared_ptr<CObject>> CreateGameSceneByServer(CDescriptorHeapManager* heapManager, const std::vector<MapGenerator::InstanceData>& instanceData);
	// Character
	void InitializeCharacterComponents(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager, const std::string& modelFileName, const std::string& animFileName,
		std::function<void(const CGeometryLoader::FrameNode*, std::shared_ptr<CMeshComponent>, std::shared_ptr<CMeshRendererComponent>)> partProcessor, bool isPlayer = false);
	void CreateUndeadCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager);
	std::shared_ptr<CCharacter> CreateReaper(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMyPlayer> CreateMyPlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CPlayer> CreatePlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMonster> CreateMonster(CDescriptorHeapManager* heapManager, MON_TYPE monType, SCENE_TYPE sceneType);

	// getter&setter
	std::vector<TreasureInfo>& GetTreauseres() { return treasures; }
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


	UndeadMeshName stringToUndeadMeshName(const std::string& str);
	LobbyMeshName stringToLobbyMeshName(const std::string& str);

	CMaterialManager matManager;
	CTextureManager texManager;
	std::map<std::string, std::shared_ptr<CObject>> prototypes;

	// 싱글모드에서만 의미있다.
	static uint32 s_monster_id_generator;

	std::vector<TreasureInfo> treasures;
};

