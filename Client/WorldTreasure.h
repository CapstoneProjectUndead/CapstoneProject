#pragma once
#include "WorldItem.h"

class CItem;

class CWorldTreasure :
    public CWorldItem
{
public:
    CWorldTreasure(std::shared_ptr<CItem> _item);
    ~CWorldTreasure();

    virtual void Update(float dt) override;
};

