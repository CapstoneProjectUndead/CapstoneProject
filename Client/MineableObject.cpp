#include "stdafx.h"
#include "MineableObject.h"

CMineableObject::CMineableObject(MINEABLEOBJECT_TYPE _type)
	: CObject(OBJECT_TYPE::MINEABLE_OBJECT)
	, type(_type)
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
