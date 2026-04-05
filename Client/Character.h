#pragma once
#include "Object.h"

class CMaterialComponent;
class CMeshComponent;

// 생성 시 Initialize 호출
class CCharacter : public CObject
{
public:
    CCharacter(OBJECT_TYPE type);

};