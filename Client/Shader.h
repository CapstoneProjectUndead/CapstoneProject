#pragma once

class CObject;
class CCamera;

/*
* texture(srv) binding order
1. srvIndex = Allocate()
2. texture->CreateTextureResource(), CreateSrv(GetCPUHandle(srvIndex))
4. SetDescriptorHeaps
3. material->texture->GetDescriptorIndex()
5. SetGraphicsRootDescriptorTable
*/
namespace DescriptorSlot {
	enum {
		DiffuseIdx = 0,
		ShadowMapIdx = 49,
		Count = 50
	};
}

struct DescriptorHeap {
	ComPtr<ID3D12DescriptorHeap> descriptor_heap;

	UINT descriptor_size{};
	UINT num_desc{};
	UINT cur_index{};

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_start{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_start{};
	UINT Allocate()
	{
		return cur_index++;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(const UINT index) const
	{
		D3D12_CPU_DESCRIPTOR_HANDLE handle = cpu_start;
		handle.ptr += index * descriptor_size;
		return handle;
	}
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(const UINT index) const
	{
		D3D12_GPU_DESCRIPTOR_HANDLE handle = gpu_start;
		handle.ptr += index * descriptor_size;
		return handle;
	}
	ID3D12DescriptorHeap* GetHeap() const { return descriptor_heap.Get(); }

	void Initialize(ID3D12Device* device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS heapFlag);
};
class CDescriptorHeapManager {
public:
	// CreateDescriptorheap
	void Initialize(ID3D12Device* device, UINT numSRV, UINT numDSV);
	// interface about SRV
	DescriptorHeap& GetSRVHeap() { return srv_heap; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUHandle(UINT index) const { return srv_heap.GetGPUHandle(index); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUHandle(UINT index) const { return srv_heap.GetCPUHandle(index); }

	// DSV 관련 인터페이스
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCPUHandle(UINT index) const { return dsv_heap.GetCPUHandle(index); }
private:
	DescriptorHeap srv_heap;
	DescriptorHeap dsv_heap;
};

/*
pipeline 상태 객체를 생성하고 게임 오브젝트를 관리하는 클래스
static shader: light만 있는 기본 쉐이더
*/
class CShader {
public:
	CShader() {};
	virtual ~CShader() {};

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	// shader code compile
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** );
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** );
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(ID3DBlob**);
	D3D12_SHADER_BYTECODE CompileShaderFromFile(WCHAR* , LPCSTR ,LPCSTR , ID3DBlob** );

	// create graphicspipeline
	virtual void CreateShader(ID3D12Device*);
	// create descriptor heap, descriptor
	virtual void CreateShaderVariables(ID3D12Device*) {}
	ID3D12RootSignature* GetGraphicsRootSignature() { return graphics_root_signature.Get(); }
	virtual ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*);

	virtual void CreateDescriptorHeap(ID3D12Device*, UINT);
	CDescriptorHeapManager* GetHeapManager() const { return heap_manager.get(); }

	virtual void RenderBegin(ID3D12GraphicsCommandList*);
	virtual void Render(ID3D12GraphicsCommandList*, CObject*);
	virtual void RenderEnd(ID3D12GraphicsCommandList*) {};

protected:
	ComPtr<ID3D12PipelineState> pipeline_states{};
	ComPtr<ID3D12RootSignature> graphics_root_signature{};
	std::unique_ptr<CDescriptorHeapManager> heap_manager;
};

// 스키닝 정보 shader
class CSkinningShader : public CShader
{
public:
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**) override;
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*) override;
};

// hardware instancing shader
class CInstShader : public CShader
{
public:
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**) override;
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*) override;
};

// CInstShader에서 CULL_MODE_NONE
class CTwoSideShader : public CInstShader
{
public:
	D3D12_RASTERIZER_DESC CreateRasterizerState() override;
};

class CShadowShader : public CSkinningShader {
public:
	D3D12_RASTERIZER_DESC CreateRasterizerState() override;
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_BLEND_DESC CreateBlendState() override;
	void CreateShader(ID3D12Device* device) override;
};

class CUIShader : public CShader
{
public:
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**) override;
	D3D12_RASTERIZER_DESC CreateRasterizerState() override;
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*) override;
};

class CBillboardShader : public CShader
{
public:
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreateGeometryShader(ID3DBlob**) override;
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*) override;
	void CreateShader(ID3D12Device*) override;
	D3D12_RASTERIZER_DESC CreateRasterizerState() override;
};