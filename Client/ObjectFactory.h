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
struct FrameNode;

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;
	void LoadFrameNode(CDescriptorHeapManager* heapManager, std::map<std::string, std::shared_ptr<CObject>>& objects, const std::unique_ptr<FrameNode>& node);
	std::vector<std::shared_ptr<CObject>> CreateLobby(CDescriptorHeapManager* heapManager);
	// GameScene 모델 파츠 load
	void LoadGameScene(CDescriptorHeapManager* heapManager);
	std::vector<std::shared_ptr<CObject>> CreateGameScene(CDescriptorHeapManager* heapManager);
	// Initialize 호출 X
	void CreateUndeadCharacter(std::shared_ptr<CCharacter> character, CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMyPlayer> CreateMyPlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CPlayer> CreatePlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMonster> CreateHumanMonster(CDescriptorHeapManager* heapManager, MON_TYPE monType, SCENE_TYPE sceneType);
	void SetComponent(std::shared_ptr<CPlayer>& player);
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
	// MapGenerator로 생성되는 grid를 model과 매치
	std::vector<std::string> GameSceneTypeToString(const MapGenerator::EModelType& type);
	std::string PickRandom(const std::string& key);

	CMaterialManager matManager;
	CTextureManager texManager;
	std::map<std::string, std::shared_ptr<CObject>> prototypes;

	// 싱글모드에서만 의미있다.
	static uint32 s_monster_id_generator;
};

