#pragma once
#include "WorldItem.h"

class CWorldWeapon :
    public CWorldItem
{
public:
    CWorldWeapon(std::shared_ptr<CItem> item);
    ~CWorldWeapon();

    virtual void Update(float dt) override;
};

