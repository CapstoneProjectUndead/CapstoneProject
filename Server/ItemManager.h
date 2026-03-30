#pragma once
#include <MapGenerator/MapGenerator.h>

//==============================
// 보물도 ItemManager에서 관리한다.
//==============================

class CItem;

struct WorldItem
{
	shared_ptr<CItem>	item;
	uint32				world_id;
	XMFLOAT3			position;

	WorldItem(shared_ptr<CItem> _item, const uint32 id, const XMFLOAT3& pos)
		: item(_item)
		, world_id(id)
		, position(pos)
	{ }
};

class CItemManager
{
	friend class CLobbyScene;
	friend class CGameScene;
public:
	CItemManager(SCENE_TYPE sceneType);
	CItemManager(const CItemManager&) = delete;
	~CItemManager();

public:
	// 아이템 도감 번호에 맞는 Item 생성
	shared_ptr<CItem> CreateItem(uint16 itemId);
	shared_ptr<WorldItem> SpawnItem(uint16 itemId, const XMFLOAT3& pos);

	// 조회 (없으면 nullptr)
	shared_ptr<CItem> FindItem(uint64 worldId);

	// 제거 (픽업/디스폰 시), 성공 여부 반환
	bool RemoveItem(uint64 worldId);

	// 전체 목록 (늦게 입장한 플레이어에게 스폰 패킷 보낼 때)
	const unordered_map<uint32, shared_ptr<WorldItem>>& GetItems() const { return items; }

private:
	// 보물 관련
	void SpawnWorldTreasures(const vector<MapGenerator::InstanceData>& instanceData);

private:
	SCENE_TYPE scene_type;
	unordered_map<uint32, shared_ptr<WorldItem>> items;
	uint32 world_id_counter;  // SpawnItem 호출 시 자동 증가

	map<uint32, TreasureInfo>           treasure_map;
};

