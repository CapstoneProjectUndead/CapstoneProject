#pragma once
#include "Material.h"
#include "Texture.h"

class CDescriptorHeapManager;
class CCharacter;
class CObject;
class CPlayer;
class CMyPlayer;
class CMonster;
class CHumanMonster;

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;
	std::vector<std::shared_ptr<CObject>> CreateLobby(CDescriptorHeapManager* heapManager);
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
		Counter,	// 카운터
		Unknown
	};


	UndeadMeshName stringToUndeadMeshName(const std::string& str);
	LobbyMeshName stringToLobbyMeshName(const std::string& str);

	CMaterialManager matManager;
	CTextureManager texManager;

	// 싱글모드에서만 의미있다.
	static uint32 s_monster_id_generator;
};

