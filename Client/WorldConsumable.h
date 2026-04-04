#pragma once
#include "WorldItem.h"

class CWorldConsumable :
    public CWorldItem
{
public:
public:
    CWorldConsumable(std::shared_ptr<CItem> item);
    ~CWorldConsumable();

    virtual void Update(float dt) override;
};

