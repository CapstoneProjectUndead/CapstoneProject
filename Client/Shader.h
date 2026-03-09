#pragma once

class CObject;
class CCamera;

/*
* 텍스처(srv) 바인딩 순서
1. srvIndex = Allocate()
2. texture->CreateTextureResource(), CreateSrv(GetCPUHandle(srvIndex))
3. 오브젝트에 SRV 인덱스 저장
4. SetDescriptorHeaps
5. SetGraphicsRootDescriptorTable
*/
class CDescriptorHeapManager {
public:
	// 디스크립터 힙 생성
	void Initialize(ID3D12Device* device, UINT numDescriptors);
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
private:
	ComPtr<ID3D12DescriptorHeap> descriptor_heap;

	UINT descriptor_size{};
	UINT num_desc{};
	UINT cur_index{};

	D3D12_CPU_DESCRIPTOR_HANDLE cpu_start{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu_start{};
};

/*
pipeline 상태 객체를 생성하고 게임 오브젝트를 관리하는 클래스
static shader: light만 있는 기본 쉐이더
*/
class CShader {
public:
	CShader() {};
	virtual ~CShader() {};

	// 입력 조립기에게 정점 버퍼를 알려주기 위한 구조체 반환
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	// 래스터라이저 상태를 설정하기 위한 구조체 반환
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	// 블렌드 상태를 설정하기 위한 구조체 반환
	virtual D3D12_BLEND_DESC CreateBlendState();
	// 깊이-스텐실 검사를 위한 상태를 설정하기 위한 구조체 반환
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	// 셰이더 코드 컴파일
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob** );
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob** );
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(ID3DBlob**);
	D3D12_SHADER_BYTECODE CompileShaderFromFile(WCHAR* , LPCSTR ,LPCSTR , ID3DBlob** );

	//그래픽스 파이프라인 상태 객체 생성
	virtual void CreateShader(ID3D12Device*);
	// descriptor heap, descriptor 생성 함수
	virtual void CreateShaderVariables(ID3D12Device*) {}
	// 루트 시그니처, CreateDescriptorHeap 수행
	ID3D12RootSignature* GetGraphicsRootSignature() { return graphics_root_signature.Get(); }
	virtual ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*);

	//
	virtual void CreateDescriptorHeap(ID3D12Device*, UINT);
	CDescriptorHeapManager* GetHeapManager() const { return heap_manager.get(); }

	virtual void RenderBegin(ID3D12GraphicsCommandList*);
	virtual void Render(ID3D12GraphicsCommandList*, CObject*);
	virtual void RenderEnd(ID3D12GraphicsCommandList*) {};

	// 임시 통로 역할
	static CDescriptorHeapManager* current_heap_manager;
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
};

// hardware instancing shader
class CInstShader : public CShader
{
public:
	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**) override;
	ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*) override;
	void Render(ID3D12GraphicsCommandList*, CObject*) override;
};