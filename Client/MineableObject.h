#pragma once
#include "Object.h"

class CMineableObject : public CObject
{
public:
    CMineableObject(MINEABLEOBJECT_TYPE _type);
    ~CMineableObject();

    virtual void Initialize() override;
    virtual void Update(const float dt) override;

    MINEABLEOBJECT_TYPE GetMineableType() const { return type; }

public:
    int GetHp() const { return hp; }
    void TakeDamage();
    bool IsDestroyed() const { return hp <= 0; }
    void DestroyImmediate() { hp = 0; }

private:
    MINEABLEOBJECT_TYPE type;
    int hp = 5;
    const int max_hp = 5;
};
