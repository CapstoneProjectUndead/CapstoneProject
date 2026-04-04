#pragma once
#include "Object.h"

class CItem;

class CWorldItem : public CObject
{
public:
	CWorldItem(std::shared_ptr<CItem> _item);
	virtual ~CWorldItem() = 0;

	virtual void Update(float dt) override;

public:
	std::shared_ptr<CItem> GetItem() const { return item; }

private:
	std::shared_ptr<CItem> item;	
	float spin_angle;				// Y축 회전 누적 각도
};
