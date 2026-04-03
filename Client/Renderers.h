#pragma once
#include "GPUBufferStruct.h"

class CMesh;
class CRectangleMesh;
class CBillboardMesh;

/*
template은 컨테이너에 사용 불가
* interface class
*/
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void Initialize(ID3D12Device* dev, UINT instSize) = 0;
    virtual void Render(ID3D12GraphicsCommandList* cmdList) = 0;
    // bool 인자로 batch 배열 선택
    virtual void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic) {};
    // white texture 사용(사용 시 힙 0번에 tex set)
    virtual void AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic) {};
    virtual void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, AnimationData aniData) {};
};

template<typename T>
class CRenderer : public IRenderer {

public:
    virtual ~CRenderer() {
        if (inst_cb) inst_cb->Unmap(0, nullptr);
    }

    // bool 인자로 batch 배열 선택
    virtual void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic) override;
    // white texture 사용(사용 시 힙 0번에 tex set)
    virtual void AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic) override;

    void Initialize(ID3D12Device* dev, UINT instSize);
    void ResizeBuffer(UINT requiredSize);
    // 데이터를 버퍼에 복사하고 Draw를 호출하는 공통 루프
    void RenderBatches(ID3D12GraphicsCommandList* cmdList, UINT rootSlot);
    virtual void Render(ID3D12GraphicsCommandList* cmdList) = 0;
protected:
    T* mapped{};
    ComPtr<ID3D12Resource> inst_cb;
    UINT max_capacity{};
    ID3D12Device* device{};

    // 한번만 설정-> static
    // 계속 변경-> dynamic
    std::map<CMesh*, std::vector<T>> static_batches;
    std::map<CMesh*, std::vector<T>> dynamic_batches;
};

class CInstRenderer : public CRenderer<InstCB> {
public:
    void Render(ID3D12GraphicsCommandList* cmdList) override;
};

class CAniRenderer : public CRenderer<AniCB> {
public:
    void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, AnimationData aniData) override;
    void Render(ID3D12GraphicsCommandList* cmdList) override;
};

class CUIRenderer : public CRenderer<UIInstCB> {
public:
    CUIRenderer();
    void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic) override;
    void AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic) override;
    void Render(ID3D12GraphicsCommandList* cmdList) override;
private:
    std::shared_ptr<CRectangleMesh> quad_mesh;
};

class CBillboardRenderer : public CRenderer<BillboardInstCB> {
public:
    CBillboardRenderer();
    void AddInstance(CMesh* mesh, CMaterialComponent* material, const XMFLOAT4X4& world, bool isStatic) override;
    void AddInstance(CMesh* mesh, const XMFLOAT4 color, const XMFLOAT4X4& world, bool isStatic) override;
    void Render(ID3D12GraphicsCommandList* cmdList) override;
private:
    std::shared_ptr<CBillboardMesh> b_mesh;
};

class CTextRenderer : public IRenderer {
public:
    // ui shader heap의 20번째 인덱스 사용(덮어버리지 않게 주의)
    void Initialize(ID3D12Device* device, ID3D12CommandQueue* commandQueue, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
    virtual void Initialize(ID3D12Device* dev, UINT instSize) override {};
    // casting 하여 사용
    void AddTextInstance(const std::wstring & str, const XMFLOAT4X4 & world, const XMFLOAT4 & color, bool billboard);
    void Render(ID3D12GraphicsCommandList* cmdList) override;
private:
    std::unique_ptr<SpriteBatch> sprite_batch;
    std::unique_ptr<SpriteFont> default_font;
    std::vector<TextInst> textes;
};