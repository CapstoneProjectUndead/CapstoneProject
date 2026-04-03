#include "stdafx.h"
#include "AnimationManager.h"
#include "GeometryLoader.h"

void CAnimationManager::Initialize(const std::string& charName, const std::string& AniName)
{
    // load skeletonData
    CGeometryLoader::SkeletonData skeleton = CGeometryLoader::LoadSkeleton(charName);
    uint32_t boneCount = (uint32_t)skeleton.bone_names.size();

    // load animation(boneMatrixes)
    animations = CGeometryLoader::LoadAnimations(AniName, boneCount);

    // 로드된 모든 클립에 bone_count 정보 기입
    for (auto& [name, clip] : animations)
    {
        clip.bone_count = boneCount;
    }
}

void CAnimationManager::CreateAnimationTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
    // 1. 모든 클립의 행렬 데이터를 하나의 연속된 메모리로 병합
    std::vector<XMFLOAT4X4> totalMatrices;
    for (auto& [name, clip] : animations)
    {
        // 각 클립의 시작 오프셋(행렬 단위)을 기록해둬야 셰이더에서 접근 가능합니다.
        clip.start_matrix_offset = (uint32_t)totalMatrices.size();
        totalMatrices.insert(totalMatrices.end(), clip.baked_matrices.begin(), clip.baked_matrices.end());
    }

    UINT totalBytes = (UINT)(totalMatrices.size() * sizeof(XMFLOAT4X4));

    // 2. 유저님의 CreateBufferResource 함수 호출
    // resourceStates는 셰이더에서 읽을 수 있도록 NON_PIXEL_SHADER_RESOURCE 등으로 설정합니다.
    texture = CreateBufferResource(
        device,
        cmdList,
        totalMatrices.data(),
        totalBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        upload_buffer.GetAddressOf()
    );

    // 3. SRV 생성 (Shader Resource View)
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = (UINT)totalMatrices.size();
    srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(texture.Get(), &srvDesc, cpuDescriptorHandle);
}