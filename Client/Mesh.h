#pragma once

namespace CGeometryLoader {
	struct FrameNode;
	struct MeshCollider;
	struct Mesh;
}

class CVertex {
public:
	CVertex() : position{ XMFLOAT3(0.0f, 0.0f, 0.0f) }, normal{ XMFLOAT3(0.0f, 1.0f, 0.0f)} {}
	CVertex(XMFLOAT3 position, XMFLOAT3 normal) : position{ position }, normal{ normal } {}
	CVertex(XMFLOAT3 position) : position{ position }, normal{ XMFLOAT3(0.0f, 1.0f, 0.0f) } {}

	XMFLOAT3 position{};
	XMFLOAT3 normal{};
};

class CMatVertex : public CVertex{
public:
	CMatVertex();
	CMatVertex(XMFLOAT3 position, XMFLOAT2 tex);
	CMatVertex(XMFLOAT3 position, XMFLOAT2 tex, XMFLOAT3 normal);

	XMFLOAT2 tex{};
};

class CSkinnedVertex : public CMatVertex {
public:
	CSkinnedVertex() : CMatVertex() {}
	CSkinnedVertex(XMFLOAT3 position, XMFLOAT2 tex, XMFLOAT3 normal);

	XMUINT4  bone_indices{};
	XMFLOAT4 bone_weights{};
};

class CBillBoardVertex {
public:
	CBillBoardVertex();
	CBillBoardVertex(XMFLOAT3 position);
	void SetPos(XMFLOAT3 pos) { position = pos; }
protected:
	XMFLOAT3 position{};
	//XMFLOAT2 size{};	// world_matrix에서 추출해서 사용
};

class CMesh
{
public:
	CMesh() {}
	CMesh(ID3D12Device*, ID3D12GraphicsCommandList*);

	void ReleaseUploadBuffer();

	virtual void Render(ID3D12GraphicsCommandList*, uint32 instCount = 1);
	void SetPrimitive(D3D12_PRIMITIVE_TOPOLOGY t) { primitive_topology = t; }

	// 불러온 모델 데이터 저장용 함수
	template<typename T>
	void SetVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<T> vertices);
	void SetIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<UINT> indices);

	template<typename T>
	void BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node);
	template<typename T>
	void BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const CGeometryLoader::MeshCollider& collider);
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
public:
	// name
	std::string name{};
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
	CCubeMesh(ID3D12Device*, ID3D12GraphicsCommandList*, const XMFLOAT3&, const XMFLOAT3& = XMFLOAT3{});
};

class CSphereMesh : public CMesh
{
public:
	CSphereMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float radius, const XMFLOAT3& pivot = XMFLOAT3{}, UINT sliceCount = 16, UINT stackCount = 16);
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


template<typename T>
void CMesh::BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	CGeometryLoader::Mesh& mesh{ node->mesh };
	name = node->name;

	std::vector<T> vertices;
	size_t count = mesh.positions.size();
	vertices.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		T v{};
		v.position = mesh.positions[i];
		v.normal = (i < mesh.normals.size()) ? mesh.normals[i] : XMFLOAT3(0, 1, 0);

		vertices.push_back(v);
	}

	SetVertices(device, commandList, (UINT)vertices.size(), vertices);
}

template<typename T>
inline void CMesh::BuildVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const CGeometryLoader::MeshCollider& collider)
{
	std::vector<T> vertices;
	size_t count = collider.positions.size();
	vertices.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		T v{};
		v.position = collider.positions[i];
		v.normal = (i < collider.normals.size()) ? collider.normals[i] : XMFLOAT3(0, 1, 0);

		vertices.push_back(v);
	}

	SetVertices(device, commandList, (UINT)vertices.size(), vertices);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
}

template<>
void CMesh::BuildVertices<CSkinnedVertex>(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node);

template<>
void CMesh::BuildVertices<CMatVertex>(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node);