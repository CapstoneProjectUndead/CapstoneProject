#pragma once
#include "Object.h"

class CMineableObject : public CObject
{
public:
    CMineableObject();
    ~CMineableObject();

    virtual void Initialize() override;
    virtual void Update(const float dt) override;

public:
    void TakeDamage();
    bool IsDestroyed() const { return hp <= 0; }
    int GetHp() const { return hp; }

private:
    int hp = 5;
    const int max_hp = 5;
};
