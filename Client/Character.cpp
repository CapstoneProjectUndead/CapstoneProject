#include "stdafx.h"
#include "Object.inl"
#include "Mesh.h"
#include "Character.h"
#include "Movement.h"
#include "Animator.h"

CCharacter::CCharacter()
	: CObject()
{
}

void CCharacter::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	// set 메쉬
	std::string fileName{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(fileName);
	for (const auto& children : frameRoot->childrens) {
		if (children->mesh.positions.empty()) break;
		SetMeshFromFile<CMatVertex>(device, commandList, children);
	}

	auto animator = std::make_shared<CAnimatorComponent>();
	animator->Initialize(fileName, "../Modeling/undead_ani.bin");
	animator->Play("Ganga_walk");
	SetComponent(animator);
	SetShdaer("skinning");

	SetComponent(std::make_shared<CMovementComponent>());

	CObject::CreateConstantBuffers(device, commandList);
}