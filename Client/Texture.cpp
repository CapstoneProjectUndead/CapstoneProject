#include "stdafx.h"
#include "Texture.h"
#include "DDSTextureLoader.h"
#include "Shader.h"

void CTexture::CreateTextureResource(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, std::wstring& fileName)
{
	ThrowIfFailed(CreateDDSTextureFromFile12(device, commandList, fileName.c_str(), texture, upload_buffer));
}

void CTexture::CreateSrv(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(texture.Get(), &srvDesc, cpuDescriptorHandle);
}

std::shared_ptr<CTexture> CTextureManager::GetTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, CDescriptorHeapManager* heap, const std::string& name)
{
    ID3D12DescriptorHeap* heapPtr = heap->GetHeap();
    auto key = std::make_pair(name, heapPtr); // 키 생성

    auto it = textures.find(key);
    if (it != textures.end())
        return it->second;

    auto tex = std::make_shared<CTexture>();

    std::wstring wname(name.begin(), name.end());
    std::wstring path = L"../Modeling/tex/" + wname + L".dds";

    tex->CreateTextureResource(device, commandList, path);

    UINT index = heap->Allocate();
    tex->SetDescriptorIndex(index);
    tex->CreateSrv(device, heap->GetCPUHandle(index));

    textures[key] = tex;

    return tex;
}