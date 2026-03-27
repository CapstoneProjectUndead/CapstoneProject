#include "stdafx.h"
#include "WorldConsumable.h"

CWorldConsumable::CWorldConsumable(std::shared_ptr<CItem> _item)
	: CWorldItem(_item)
{
}

CWorldConsumable::~CWorldConsumable()
{
}

void CWorldConsumable::Update(float dt)
{
}