#pragma once

class CObject;
class CCamera;

// pipeline 상태 객체를 생성하고 게임 오브젝트를 관리하는 클래스
class CShader {
public:
	CShader();
	virtual ~CShader();
	void ReleaseUploadBuffer();
	virtual void BuildObjects(ID3D12Device*, ID3D12GraphicsCommandList*);

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
	virtual void CreateShader(ID3D12Device*, ID3D12RootSignature*);
	// descriptor heap, descriptor 생성 함수
	virtual void CreateShaderVariables(ID3D12Device*) {}

	void Animate(float elapsedTime, CCamera* camera);

	virtual void Render(ID3D12GraphicsCommandList*);
protected:
	std::deque<ComPtr<ID3D12PipelineState>> pipeline_states{};
	std::deque<std::unique_ptr<CObject>> objects{};
};

class CTextureShader final : public CShader{
public:
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout() override;

	// 블렌드 상태를 설정하기 위한 구조체 반환
	D3D12_BLEND_DESC CreateBlendState() override;
	// 깊이-스텐실 검사를 위한 상태를 설정하기 위한 구조체 반환
	D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() override;

	D3D12_SHADER_BYTECODE CreateVertexShader(ID3DBlob**) override;
	D3D12_SHADER_BYTECODE CreatePixelShader(ID3DBlob**)override;
	D3D12_SHADER_BYTECODE CreateGeometryShader(ID3DBlob**) override;
	void CreateShader(ID3D12Device* device, ID3D12RootSignature*) override;

	ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device*);
	void CreateShaderVariables(ID3D12Device*) override;

	void Render(ID3D12GraphicsCommandList*) override;
private:
	ComPtr<ID3D12DescriptorHeap> descriptor_heap;
};
