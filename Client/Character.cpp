#include "stdafx.h"
#include "Character.h"
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
		auto mesh = std::make_shared<CMesh>();
		if (children->mesh.positions.empty()) break;

		mesh->BuildVertices<CVertex>(device, commandList, children->mesh);
		mesh->SetIndices(device, commandList, (UINT)children->mesh.indices.size(), children->mesh.indices);
		SetMesh(mesh);
		//world_matrix = children->localMatrix;
	}

	// animator 초기화
	//animator = std::make_unique<CAnimator>();
	//animator->Initialize(fileName, std::string("../Modeling/undead_ani.bin"));
	//animator->Play("Ganga_walk");   // 초기 애니메이션

	CreateConstantBuffers(device, commandList);
}

void CCharacter::Update(float deltaTime)
{
	CObject::Update(deltaTime);

	if (animator) {
		/*if (Vector3::Length(velocity) <= 0.0f)
			animator->Play("Ganga_idle");
		else
			animator->Play("Ganga_walk");*/

		animator->Update(deltaTime);
	}
}

void CCharacter::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	CObject::UpdateShaderVariables(commandList);

	if (animator) animator->UpdateShaderVariables(commandList);
}

void CCharacter::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CObject::CreateConstantBuffers(device, commandList);

	if (animator) animator->CreateConstantBuffers(device, commandList);
}
