#pragma once

class CDescriptorHeapManager;

class CTexture {
public:
	CTexture() = default;
	void CreateTextureResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList ,std::wstring& fileName);
	void CreateSrv(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle);

	ID3D12Resource* GetTextureResource() const { return texture.Get(); }
	void SetDescriptorIndex(UINT index) { descriptor_index = index; }
	UINT GetDescriptorIndex() const { return descriptor_index; }
private:
	ComPtr<ID3D12Resource> texture;
	ComPtr<ID3D12Resource> upload_buffer;
	int descriptor_index{};
};

class CTextureManager
{
public:
	// 없으면 생성
	std::shared_ptr<CTexture> GetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmd, CDescriptorHeapManager* heap, const std::string& name);
private:
    std::unordered_map<std::string, std::shared_ptr<CTexture>> textures;
};
