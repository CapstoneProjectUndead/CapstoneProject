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
	std::vector<XMFLOAT4X4> baked_matrices;
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

	AnimationClip GetClip(const std::string name) { return animations[name]; }
	ID3D12Resource* GetTextureResource() const { return texture.Get(); }
private:
	ComPtr<ID3D12Resource> texture;
	ComPtr<ID3D12Resource> upload_buffer;
	std::unordered_map<std::string, AnimationClip> animations;
};