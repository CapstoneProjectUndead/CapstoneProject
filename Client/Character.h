#pragma once
#include "Object.h"
#include "SkinnedData.h"

class CMaterialComponent;
class CMeshComponent;

// 생성 시 Initialize 호출
class CCharacter : public CObject
{
public:
    CCharacter(OBJECT_TYPE type);

	void ChangeModelSet(int setIndex);
	void ChangeEyes(int index);
	void ChangeMouth(int index);

	std::array<std::shared_ptr<CMaterialComponent>, 3> body_materials;
	std::array<std::vector<std::shared_ptr<CMeshComponent>>, 3> eartail_parts;
	std::array<std::shared_ptr<CMaterialComponent>, 3> eyes_material;
	std::array<std::shared_ptr<CMaterialComponent>, 3> mouth_material;
};