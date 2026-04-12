#include "stdafx.h"
#include "WorldConsumable.h"

CWorldConsumable::CWorldConsumable(std::shared_ptr<CItem> item)
	: CWorldItem(item)
{
}

CWorldConsumable::~CWorldConsumable()
{
}

void CWorldConsumable::Update(float dt)
{
	CWorldItem::Update(dt);
}