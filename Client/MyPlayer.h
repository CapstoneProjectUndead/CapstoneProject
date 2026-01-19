#pragma once
#include "Player.h"

class CMyPlayer :
    public CPlayer
{
public:
    using CObject::Move; // 부모의 Move 모두 노출

    CMyPlayer();
    ~CMyPlayer() {};

    virtual void Update(float elapsedTime) override;

    void ProcessInput();
};

