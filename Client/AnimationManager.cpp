#include "stdafx.h"
#include "AnimationManager.h"
#include "GeometryLoader.h"

void CAnimationManager::SetRecursiveWeight(int boneIdx, float weight, std::vector<float>& maskWeights, const CGeometryLoader::SkeletonData& skeleton)
{
    if (boneIdx < 0 || boneIdx >= maskWeights.size()) return;

    // 현재 본의 가중치 설정
    maskWeights[boneIdx] = weight;

    // 모든 본을 순회하며 현재 본을 부모로 가진 자식들을 찾아 재귀 호출
    for (size_t i = 0; i < skeleton.parent_index.size(); ++i) {
        if (skeleton.parent_index[i] == boneIdx) {
            SetRecursiveWeight((int)i, weight, maskWeights, skeleton);
        }
    }
}

// 상반신 마스크 생성 함수
BoneMask CAnimationManager::CreateUpperBodyMask(const CGeometryLoader::SkeletonData& skeleton)
{
    uint32_t boneCount = (uint32_t)skeleton.bone_names.size();
    BoneMask mask;
    mask.name = "UpperBody";
    mask.weights.resize(boneCount, 0.0f); // 기본값은 0.0 (영향 없음)

    // spine 아래 가중치 1로 설정
    int spineIdx = skeleton.GetBoneIndex("spine");
    if (spineIdx != -1)
        SetRecursiveWeight(spineIdx, 1.0f, mask.weights, skeleton);

    return mask;
}

void CAnimationManager::Initialize(const std::string& charName, const std::string& AniName, OBJECT_TYPE objType, uint8_t subType)
{
    // load skeletonData
    CGeometryLoader::SkeletonData skeleton = CGeometryLoader::LoadSkeleton(charName);
    uint32_t boneCount = (uint32_t)skeleton.bone_names.size();

    ModelKey key{ objType, subType };
    switch (objType) {
    case OBJECT_TYPE::PLAYER:
    {
        bone_indices[key]["head"] = skeleton.GetBoneIndex("head");
        bone_indices[key]["handR"] = skeleton.GetBoneIndex("handR");
        bone_indices[key]["handL"] = skeleton.GetBoneIndex("handL");
    }
        break;
    case OBJECT_TYPE::MONSTER:
    {
        bone_indices[key]["handR"] = skeleton.GetBoneIndex("handR");
    }
        break;
    }

    bone_masks.push_back(CreateUpperBodyMask(skeleton));

    // load animation(boneMatrixes)
    auto newAnimations = CGeometryLoader::LoadAnimations(AniName, boneCount);
    for (auto& [name, clip] : newAnimations)
    {
        clip.bone_count = boneCount;
        animations[name] = std::move(clip);
    }
}

void CAnimationManager::CreateAnimationTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
    // 모든 클립의 행렬 데이터를 하나의 연속된 메모리로 병합
    std::vector<XMFLOAT4X4> totalMatrices;
    for (auto& [name, clip] : animations)
    {
        clip.start_matrix_offset = (uint32_t)totalMatrices.size();
        totalMatrices.insert(totalMatrices.end(), clip.baked_matrices.begin(), clip.baked_matrices.end());
    }

    if (totalMatrices.empty()) return;

    UINT totalBytes = (UINT)(totalMatrices.size() * sizeof(XMFLOAT4X4));

    // resourceStates는 셰이더에서 읽을 수 있도록 NON_PIXEL_SHADER_RESOURCE 등으로 설정
    texture = CreateBufferResource(
        device,
        cmdList,
        totalMatrices.data(),
        totalBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        upload_buffer.GetAddressOf()
    );

    // SRV 생성
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = (UINT)totalMatrices.size();
    srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(texture.Get(), &srvDesc, cpuDescriptorHandle);

    // gpu 등록 후 cpu에서 제거
    for (auto& [name, clip] : animations) {
        clip.ClearGPUMemory();
    }
}

void CAnimationManager::CreateMaskBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
    if (bone_masks.empty()) return;

    std::vector<float> totalWeight;
    for (auto& mask : bone_masks)
    {
        totalWeight.insert(totalWeight.end(), mask.weights.begin(), mask.weights.end());
    }

    UINT totalBytes = (UINT)(totalWeight.size() * sizeof(float));

    mask_buffer = CreateBufferResource(
        device,
        cmdList,
        totalWeight.data(),
        totalBytes,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        mask_upload_buffer.GetAddressOf()
    );

    // SRV 생성
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = (UINT)totalWeight.size();
    srvDesc.Buffer.StructureByteStride = sizeof(float);
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    device->CreateShaderResourceView(mask_buffer.Get(), &srvDesc, cpuDescriptorHandle);

    for (auto& mask : bone_masks) {
        mask.name.clear();
        mask.name.shrink_to_fit();
        mask.weights.clear();
        mask.weights.shrink_to_fit();
    }
}

XMMATRIX CAnimationManager::GetBoneSocketMatrix(const std::string& clipName, float currentTime, int boneIdx)
{
    auto it = animations.find(clipName);
    if (it == animations.end() || boneIdx < 0) return XMMatrixIdentity();

    AnimationClip& clip = it->second;

    // 현재 시간에 따른 프레임 인덱스 및 보간 계수(t) 계산
    float frameFloat = currentTime * 60.0f; // 60fps 기준
    int frame0 = (int)frameFloat % clip.total_frames;
    int frame1 = (frame0 + 1) % clip.total_frames;
    float t = frameFloat - (int)frameFloat;

    // 현재 프레임과 다음 프레임의 ToRoot 행렬 인덱스 계산
    int idx0 = frame0 * clip.bone_count + boneIdx;
    int idx1 = frame1 * clip.bone_count + boneIdx;

    // 두 행렬 사이를 보간 (위치, 회전, 스케일을 분해해서 섞어야 정확함)
    XMMATRIX m0 = XMLoadFloat4x4(&clip.toRoot_matrices[idx0]);
    XMMATRIX m1 = XMLoadFloat4x4(&clip.toRoot_matrices[idx1]);
    m0 = XMMatrixTranspose(m0);
    m1 = XMMatrixTranspose(m1);
    
    XMVECTOR s0, r0, t0;
    XMVECTOR s1, r1, t1;
    XMMatrixDecompose(&s0, &r0, &t0, m0);
    XMMatrixDecompose(&s1, &r1, &t1, m1);

    // 선형 보간 (Lerp / Slerp)
    XMVECTOR S = XMVectorLerp(s0, s1, t);
    XMVECTOR R = XMQuaternionSlerp(r0, r1, t);
    XMVECTOR T = XMVectorLerp(t0, t1, t);

    // 보간된 로컬(ToRoot) 행렬 완성
    XMMATRIX interpolatedToRoot = XMMatrixAffineTransformation(S, XMVectorZero(), R, T);

    return interpolatedToRoot;
}