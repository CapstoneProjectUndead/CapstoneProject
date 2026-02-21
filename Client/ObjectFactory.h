#pragma once
#include "Material.h"
#include "Texture.h"

class CDescriptorHeapManager;
class CCharacter;
class CObject;
class CPlayer;
class CMyPlayer;

class CObjectFactory
{
public:
	CObjectFactory() = default;
	~CObjectFactory() = default;
	std::vector<std::shared_ptr<CObject>> CreateLobby(CDescriptorHeapManager* heapManager);
	// Initialize 호출 X
	void CreateUndeadCharacter(std::shared_ptr<CPlayer> character, CDescriptorHeapManager* heapManager);
	std::shared_ptr<CMyPlayer> CreateMyPlayer(CDescriptorHeapManager* heapManager);
	std::shared_ptr<CPlayer> CreatePlayer(CDescriptorHeapManager* heapManager);
private:
	enum class MeshName {
		body,
		Bunny_ear,
		Bunny_tail,
		Cat_ear,
		Cat_tail,
		Dog_ear,
		Dog_tail,
		eyes,
		mouse,
		Unknown,
	};

	MeshName stringToMeshName(const std::string& str);

	CMaterialManager matManager;
	CTextureManager texManager;
};

