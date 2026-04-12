#include "stdafx.h"
#include "WorldTool.h"

CWorldTool::CWorldTool(std::shared_ptr<CItem> item)
	: CWorldItem(item)
{
}

CWorldTool::~CWorldTool()
{
}

void CWorldTool::Update(float dt)
{
	CWorldItem::Update(dt);
}
