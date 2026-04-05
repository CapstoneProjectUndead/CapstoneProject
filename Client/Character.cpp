#include "stdafx.h"
#include "Character.h"
#include "Animator.h"
#include "Material.h"
#include "MeshRenderer.h"

CCharacter::CCharacter(OBJECT_TYPE type)
	: CObject(type)
{
}

void CCharacter::ChangeModelSet(int setIndex)
{
    for (int i = 0; i < eartail_parts.size(); ++i) {
        bool active = (i == setIndex);

        body_materials[i]->SetEnable(active);

        for (auto& mesh : eartail_parts[i]) {
            mesh->SetEnable(active);
        }
    }
}

void CCharacter::ChangeEyes(int index)
{
    for (int i = 0; i < eyes_material.size(); ++i) {
        eyes_material[i]->SetEnable(i == index);
    }
}

void CCharacter::ChangeMouth(int index)
{
    for (int i = 0; i < mouth_material.size(); ++i) {
        mouth_material[i]->SetEnable(i == index);
    }
}