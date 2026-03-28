#pragma once
#include "Component.h"
#include <MapGenerator/MapGenerator.h>

class CItemFinder : public CComponent
{
public:
	CItemFinder();
	~CItemFinder();

	virtual void Initialize();
	virtual void Update(const float deltaTime);

public:
	std::vector<TreasureInfo>& GetTreasuerPositions() { return treasures; }

	// 맵 데이터에서 보물 위치만 추출하여 등록하는 함수
	void RegisterTreasures(const std::vector<MapGenerator::InstanceData>& mapData);
	void RegisterTreasures(const std::vector<TreasureInfo>& _treasures);
	void AddTreasure(const TreasureInfo& treasure) { treasures.push_back(treasure); }

	// F키를 눌렀을 때 주변(radius 반경 내)에 보물이 있는지 탐색하는 함수
	// 반환값: 가장 가까운 보물까지의 거리 (없으면 -1.0f 반환)
	float SearchNearbyTreasure(const XMFLOAT3& playerPos);

private:
	std::vector<TreasureInfo>  treasures;  // 보물 위치 정보
	const float				   recog_range = 5.f;  // 다우징 인지 범위
	XMFLOAT3				   closest_treasure_pos;
};

