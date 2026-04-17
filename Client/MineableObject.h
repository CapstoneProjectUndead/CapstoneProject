#pragma once
#include "Object.h"

class CMineableObject : public CObject
{
public:
    CMineableObject();
    ~CMineableObject();

    virtual void Initialize() override;
    virtual void Update(const float dt) override;

    void TakeDamage();
    bool IsDestroyed() const { return hp <= 0; }
    int GetHp() const { return hp; }

private:
    int hp{ 5 };
    int max_hp{ 5 };
};
