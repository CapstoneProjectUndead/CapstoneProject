#pragma once
#include "WorldItem.h"

class CWorldTool :
    public CWorldItem
{
public:
    CWorldTool(std::shared_ptr<CItem> item);
    ~CWorldTool();

    virtual void Update(float dt) override;
};

