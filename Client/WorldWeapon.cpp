#include "stdafx.h"
#include "WorldWeapon.h"

CWorldWeapon::CWorldWeapon(std::shared_ptr<CItem> item)
	: CWorldItem(item)
{
}

CWorldWeapon::~CWorldWeapon()
{
}

void CWorldWeapon::Update(float dt)
{
	CWorldItem::Update(dt);
}