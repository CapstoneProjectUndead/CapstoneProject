#pragma once
class CVertex {
public:
	CVertex() : position{ XMFLOAT3(0.0f, 0.0f, 0.0f) }, color{ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) } {}
	CVertex(XMFLOAT3 position, XMFLOAT4 color) : position{ position }, color{color} {}

	XMFLOAT3 position{};
	XMFLOAT4 color{};
};

class CDiffuseVertex : public CVertex{
public:
	CDiffuseVertex();
	CDiffuseVertex(XMFLOAT3 position, XMFLOAT4 color, XMFLOAT2 tex);

	XMFLOAT2 tex{};
};

class CMatVertex : public CVertex{
public:
	CMatVertex() : CVertex() {}
	CMatVertex(XMFLOAT3 position, XMFLOAT4 color, XMFLOAT3 normal);

	XMFLOAT3 normal{};
	XMUINT4  bone_indices;
	XMFLOAT4 bone_weights;
};

class CBillBoardVertex {
public:
	CBillBoardVertex();
	CBillBoardVertex(XMFLOAT3 position, XMFLOAT2 size);
	void SetPos(XMFLOAT3 pos) { position = pos; }
protected:
	XMFLOAT3 position{};
	XMFLOAT2 size{};
};

class CMesh
{
public:
	CMesh() {}
	CMesh(ID3D12Device*, ID3D12GraphicsCommandList*);

	void ReleaseUploadBuffer();

	virtual void Render(ID3D12GraphicsCommandList*);

	// 불러온 모델 데이터 저장용 함수
	template<typename T>
	void SetVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<T> vertices);
	void SetIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<UINT> indices);
protected:
	// 정점 버퍼
	ComPtr<ID3D12Resource> vertex_buffer{};
	ComPtr<ID3D12Resource> vertex_upload_buffer{};

	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{};
	UINT vertex_num{};

	// 인덱스 버퍼
	ComPtr<ID3D12Resource> index_buffer{};
	ComPtr<ID3D12Resource> index_upload_buffer{};

	D3D12_INDEX_BUFFER_VIEW index_buffer_view{};
	UINT index_num{};
	UINT start_index{};	// 인덱스 버퍼 시작 인덱스
	int base_vertex_index{}; // 인덱스 버퍼의 인덱스에 더해질 인덱스

	// View
	D3D12_PRIMITIVE_TOPOLOGY primitive_topology{ D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
	UINT slot_num{};
	UINT stride{};
	UINT offset{};
};

class CTriangleMesh : public CMesh
{
public:
	CTriangleMesh(ID3D12Device*, ID3D12GraphicsCommandList*);
};

class CRectangleMesh : public CMesh
{
public:
	CRectangleMesh(ID3D12Device*, ID3D12GraphicsCommandList*);
	CRectangleMesh(ID3D12Device*, ID3D12GraphicsCommandList*, float, float);
};

class CBillboardMesh : public CMesh {
public:
	CBillboardMesh(ID3D12Device*, ID3D12GraphicsCommandList*);
	CBillboardMesh(ID3D12Device*, ID3D12GraphicsCommandList*, float, float);

};

class CCubeMesh : public CMesh
{
public:
	CCubeMesh(ID3D12Device*, ID3D12GraphicsCommandList*, float = 2.0f, float = 2.0f, float = 2.0f);
};

template<typename T>
void CMesh::SetVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<T> vertices)
{
	vertex_num = num;
	stride = sizeof(T);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices.data(), stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}