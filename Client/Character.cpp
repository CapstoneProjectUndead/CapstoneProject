#include "stdafx.h"
#include "Character.h"

CCharacter::CCharacter()
	: CObject()
{
}

void CCharacter::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	std::string filename{ "../Modeling/undead_char.bin" };
	auto frameRoot = CGeometryLoader::LoadGeometry(filename, device, commandList);
	SetMesh(frameRoot->mesh);

	auto skeleton = CGeometryLoader::LoadSkeleton(filename);

	auto animData = CGeometryLoader::LoadAnimations(filename, skeleton.bone_names.size());
	skinned.Set(skeleton.parent_index, skeleton.inverse_bind_pose, animData);
}

void CCharacter::Update(float deltaTime)
{
    if (current_animation.empty())
        return;

	/*if (Vector3::Length(velocity) > 0.0f) {
		current_animation = std::string("Ganga_walk");
		current_time = 0.0f;
	}*/

    // 1) 시간 증가
	current_time += deltaTime;

    float start = skinned.GetClipStartTime(current_animation);
    float end = skinned.GetClipEndTime(current_animation);

    // 2) 루프 처리
    if (current_time > end)
		current_time = start;

    // 3) 본 행렬 계산
	final_transforms.resize(skinned.BoneCount());
    skinned.GetFinalTransforms(current_animation, current_time, final_transforms);
}

void CCharacter::UpdateShaderVariables(ID3D12GraphicsCommandList* commandList)
{
	CObject::UpdateShaderVariables(commandList);

	SkinnedDataCB cb{};
	if (!final_transforms.empty()) {
		UINT boneSize = skinned.BoneCount();
		for (UINT i = 0; i < boneSize; ++i)
			cb.boneTransforms[i] = final_transforms[i];
	}

	UINT8* mapped = nullptr;
	skinned_cb->Map(0, nullptr, reinterpret_cast<void**>(&mapped));
	memcpy(mapped, &cb, sizeof(cb));
	skinned_cb->Unmap(0, nullptr);

	commandList->SetGraphicsRootConstantBufferView(4, skinned_cb->GetGPUVirtualAddress());
}

void CCharacter::CreateConstantBuffers(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
	CObject::CreateConstantBuffers(device, commandList);

	SkinnedDataCB cb{};
	skinned_cb = CreateBufferResource(device, commandList, &cb, CalculateConstant<SkinnedDataCB>(), D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ, nullptr);
}
