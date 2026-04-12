#include "stdafx.h"
#include "WorldTreasure.h"

CWorldTreasure::CWorldTreasure(std::shared_ptr<CItem> item)
	: CWorldItem(item)
{
}

CWorldTreasure::~CWorldTreasure()
{
}

void CWorldTreasure::Update(float dt)
{
	CWorldItem::Update(dt);
}
