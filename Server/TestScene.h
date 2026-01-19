#pragma once
// ServerÂÊ TestScene
#include "Scene.h"

class CTestScene :
    public CScene
{
public:
    CTestScene();
    ~CTestScene();

    virtual void Update(float elapsedTime) override;
};

