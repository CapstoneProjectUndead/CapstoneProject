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
	std::vector<XMFLOAT3>& GetTreasuerPositions() { return treasure_positions; }

	// 맵 데이터에서 보물 위치만 추출하여 등록하는 함수
	void RegisterTreasures(const std::vector<MapGenerator::InstanceData>& mapData);

	// F키를 눌렀을 때 주변(radius 반경 내)에 보물이 있는지 탐색하는 함수
	// 반환값: 가장 가까운 보물까지의 거리 (없으면 -1.0f 반환)
	float SearchNearbyTreasure(const XMFLOAT3& playerPos, float radius);

private:
	std::vector<XMFLOAT3> treasure_positions;
};

