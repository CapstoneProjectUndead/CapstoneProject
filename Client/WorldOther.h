#pragma once
#include "WorldItem.h"

class CWorldOther :
    public CWorldItem
{
public:
    CWorldOther(std::shared_ptr<CItem> item);
    ~CWorldOther();

    virtual void Update(float dt) override;
};

