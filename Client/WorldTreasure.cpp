#include "stdafx.h"
#include "WorldTreasure.h"

CWorldTreasure::CWorldTreasure(std::shared_ptr<CItem> _item)
	: CWorldItem(_item)
{
}

CWorldTreasure::~CWorldTreasure()
{
}

void CWorldTreasure::Update(float dt)
{
}
