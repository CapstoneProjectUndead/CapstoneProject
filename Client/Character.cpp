#include "stdafx.h"
#include "MeshComponent.inl"
#include "Character.h"
#include "Movement.h"
#include "Animator.h"
#include "Mesh.h"

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
		// mesh
		auto meshComp = std::make_shared<CMeshComponent>();
		SetComponent(meshComp);
		meshComp->SetMeshFromFile<CSkinnedVertex>(device, commandList, children);
		world_matrix = children->localMatrix;
	}

	// animator
	auto animator = std::make_shared<CAnimatorComponent>();
	animator->Initialize(fileName, "../Modeling/undead_ani.bin");
	animator->Play("Ganga_walk");
	SetComponent(animator);
	SetShdaer("skinning");

	// movement
	SetComponent(std::make_shared<CMovementComponent>());

	CObject::Initialize(device, commandList);
}