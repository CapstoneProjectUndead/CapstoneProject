#include "stdafx.h"
#include "MineableObject.h"

CMineableObject::CMineableObject()
	: CObject(OBJECT_TYPE::MINEABLE_OBJECT)
{
}

CMineableObject::~CMineableObject()
{
}

void CMineableObject::Initialize()
{
	CObject::Initialize();
	hp = max_hp;
}

void CMineableObject::Update(const float dt)
{
	CObject::Update(dt);
}

void CMineableObject::TakeDamage()
{
	if (hp <= 0) return;
	--hp;
}
