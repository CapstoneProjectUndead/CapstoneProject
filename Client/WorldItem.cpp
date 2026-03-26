#include "stdafx.h"
#include "WorldItem.h"
#include "Item.h"

static constexpr float SPIN_SPEED = 90.0f; // 초당 90도 회전

CWorldItem::CWorldItem(std::shared_ptr<CItem> _item)
	: CObject(OBJECT_TYPE::WORLD_ITEM)
	, item(_item)
	, spin_angle(0.0f)
{
}

CWorldItem::~CWorldItem()
{
}

void CWorldItem::Update(float dt)
{
	// 아이템이 Y축으로 천천히 회전
	spin_angle += SPIN_SPEED * dt;
	if (spin_angle >= 360.0f)
		spin_angle -= 360.0f;

	Rotate(0.0f, spin_angle, 0.0f);
}
