#pragma once

struct AnimationClip
{
	std::string name;
	float clip_length;
	int total_frames;
	// 모든 프레임, 모든 본의 행렬을 일렬로 저장 (Row: Frame, Column: Bone)
	// 데이터 크기: total_frames * boneCount * sizeof(XMFLOAT4X4)
	uint32_t start_matrix_offset;
	uint32_t bone_count;          // 클립의 본 개수
	int head_bone_idx{ -1 };

	// GPU용. 쉐이더 전달 후 clear()
	std::vector<XMFLOAT4X4> baked_matrices;

	// CPU용. 머리 등 위치 계산을 위한
	std::vector<XMFLOAT4X4> toRoot_matrices;

	void ClearGPUMemory() {
		baked_matrices.clear();
		baked_matrices.shrink_to_fit(); // 실제 할당된 메모리까지 해제
	}
};

class CAnimationManager {
private:
	CAnimationManager() = default;
	CAnimationManager(const CAnimationManager&) = delete;

public:
	~CAnimationManager() {};

	static CAnimationManager& GetInstance() {
		static CAnimationManager instance;
		return instance;
	}
public:
	void Initialize(const std::string& charName, const std::string& AniName);
	void CreateAnimationTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);
	// 특정 본(머리 등)의 현재 월드 위치를 반환하는 함수
	XMVECTOR GetBoneWorldPos(const std::string& clipName, float currentTime, int boneIdx);

	AnimationClip GetClip(const std::string name) { return animations[name]; }
	ID3D12Resource* GetTextureResource() const { return texture.Get(); }
private:
	ComPtr<ID3D12Resource> texture;
	ComPtr<ID3D12Resource> upload_buffer;
	std::unordered_map<std::string, AnimationClip> animations;
};