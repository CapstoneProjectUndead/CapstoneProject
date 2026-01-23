#pragma once

class CObject;
class CCamera;

// pipeline 상태 객체를 생성하고 게임 오브젝트를 관리하는 클래스
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
	// 루트 시그니처
	ID3D12RootSignature* GetGraphicsRootSignature() { return graphics_root_signature.Get(); }
	virtual ID3D12RootSignature* CreateGraphicsRootSignature(ID3D12Device*);

	virtual void RenderBegin(ID3D12GraphicsCommandList*);
	virtual void Render(ID3D12GraphicsCommandList*, CObject*);
	virtual void RenderEnd(ID3D12GraphicsCommandList*) {};
protected:
	ComPtr<ID3D12PipelineState> pipeline_states{};
	ComPtr<ID3D12RootSignature> graphics_root_signature{};
};