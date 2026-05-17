#pragma once
#include "Object.h"

enum class MINEABLEOBJECT_TYPE
{
    VISIBLE,
    NONE_VISIBLE
};

class CMineableObject : public CObject
{
public:
    CMineableObject(MINEABLEOBJECT_TYPE _type);
    ~CMineableObject();

    virtual void Initialize() override;
    virtual void Update(const float dt) override;

public:
    void TakeDamage();
    bool IsDestroyed() const { return hp <= 0; }
    int GetHp() const { return hp; }

private:
    MINEABLEOBJECT_TYPE type;
    int hp = 5;
    const int max_hp = 5;
};
