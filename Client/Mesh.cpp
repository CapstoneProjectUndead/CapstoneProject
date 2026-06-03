#include "stdafx.h"
#include "GeometryLoader.h"
#include "Mesh.h"

// CMatVertex
CMatVertex::CMatVertex()
	: CVertex(), tex{}
{
}

CMatVertex::CMatVertex(XMFLOAT3 position, XMFLOAT2 tex)
	: CVertex(position), tex{ tex }
{
}

CMatVertex::CMatVertex(XMFLOAT3 position, XMFLOAT2 tex, XMFLOAT3 normal)
	: CVertex(position, normal), tex{ tex }
{
}

// CSkinnedVertex
CSkinnedVertex::CSkinnedVertex(XMFLOAT3 position, XMFLOAT2 tex, XMFLOAT3 normal)
	:CMatVertex(position, tex, normal)
{
}

// CBillBoardVertex
CBillBoardVertex::CBillBoardVertex()
{
}

CBillBoardVertex::CBillBoardVertex(XMFLOAT3 position)
	: position{position}
{

}

// CMesh
CMesh::CMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
}

void CMesh::ReleaseUploadBuffer()
{
	if (vertex_upload_buffer) vertex_upload_buffer.Reset();
	if (shadow_vertex_upload_buffer) shadow_vertex_upload_buffer.Reset();
}

void CMesh::Render(ID3D12GraphicsCommandList* commandList, uint32 instCount)
{
	Render(commandList, 0, instCount);
}

void CMesh::Render(ID3D12GraphicsCommandList* commandList, UINT submeshIndex, uint32 instCount)
{
	// 프리미티브 유형 설정
	commandList->IASetPrimitiveTopology(primitive_topology);
	// 정점 버퍼 뷰 설정
	commandList->IASetVertexBuffers(slot_num, 1, &vertex_buffer_view);
	if (index_buffer) {
		auto& sm = submeshes[submeshIndex];
		commandList->IASetIndexBuffer(&index_buffer_view);
		commandList->DrawIndexedInstanced(sm.index_count, instCount, sm.start_index, base_vertex_index, 0);
	}
	else {
		// 렌더링(입력 조립기 작동)
		commandList->DrawInstanced(vertex_num, instCount, offset, 0);
	}
}

void CMesh::RenderShadow(ID3D12GraphicsCommandList* commandList, uint32 instCount)
{
	RenderShadow(commandList, 0, instCount);
}

void CMesh::RenderShadow(ID3D12GraphicsCommandList* commandList, UINT submeshIndex, uint32 instCount)
{
	commandList->IASetPrimitiveTopology(primitive_topology);

	commandList->IASetVertexBuffers(slot_num, 1, &shadow_vertex_buffer_view);

	if (index_buffer) {
		auto& sm = submeshes[submeshIndex];
		commandList->IASetIndexBuffer(&index_buffer_view); // 인덱스는 공유
		commandList->DrawIndexedInstanced(sm.index_count, instCount, sm.start_index, base_vertex_index, 0);
	}
	else {
		commandList->DrawInstanced(vertex_num, instCount, offset, 0);
	}
}

void CMesh::SetIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<UINT> indices)
{
	index_num = num;
	if (submeshes.empty()) {
		submeshes.push_back({ 0, num, 0 });
	}

	index_buffer = CreateBufferResource(device, commandList, indices.data(), sizeof(UINT) * index_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, index_upload_buffer.GetAddressOf());

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
	index_buffer_view.SizeInBytes = sizeof(UINT) * index_num;
}

void CMesh::SetSubMesh(const std::vector<CGeometryLoader::SubMesh>& submesh)
{
	submeshes.clear();

	for (const auto& sm : submesh)
	{
		SubMesh sub{};

		sub.start_index = sm.start_index;
		sub.index_count = sm.index_count;
		sub.material_index = sm.material_index;

		submeshes.push_back(sub);
	}
}

template<>
void CMesh::BuildVertices<CMatVertex>(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	CGeometryLoader::Mesh& mesh{ node->mesh };
	name = node->name;

	// [기존 로직] 일반 렌더링용 정점 생성
	std::vector<CMatVertex> vertices;
	std::vector<CShadowVertex> shadowVertices;

	size_t count = mesh.positions.size();
	vertices.reserve(count);
	shadowVertices.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		// 일반 정점
		CMatVertex v{};
		v.position = mesh.positions[i];
		v.normal = (i < mesh.normals.size()) ? mesh.normals[i] : XMFLOAT3(0, 1, 0);
		v.tex = (i < mesh.texcoords.size()) ? mesh.texcoords[i] : XMFLOAT2(0, 0);
		vertices.push_back(v);

		// 그림자 정점 (오직 Position만 추출)
		CShadowVertex sv{};
		sv.position = mesh.positions[i];
		shadowVertices.push_back(sv);
	}

	for (const auto& m : mesh.materials) {
		if (!m.normalMap.empty()) {
			CalculateTangents<CMatVertex>(vertices, mesh.indices);
			break;
		}
	}

	// 일반 버퍼 세팅
	SetVertices(device, commandList, (UINT)vertices.size(), vertices);

	// 그림자 버퍼 세팅 (메모리 업로드 및 뷰 생성)
	shadow_stride = sizeof(CShadowVertex);
	shadow_vertex_buffer = CreateBufferResource(device, commandList, shadowVertices.data(), shadow_stride * (UINT)shadowVertices.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, shadow_vertex_upload_buffer.GetAddressOf());

	shadow_vertex_buffer_view.BufferLocation = shadow_vertex_buffer->GetGPUVirtualAddress();
	shadow_vertex_buffer_view.StrideInBytes = shadow_stride;
	shadow_vertex_buffer_view.SizeInBytes = shadow_stride * (UINT)shadowVertices.size();
}

// 2. 스킨드 메쉬 (CSkilledVertex) 빌드 시 그림자 버퍼 생성
template<>
void CMesh::BuildVertices<CSkinnedVertex>(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	CGeometryLoader::Mesh& mesh{ node->mesh };
	name = node->name;

	std::vector<CSkinnedVertex> vertices;
	std::vector<CShadowSkinnedVertex> shadowVertices; // 스킨드 그림자용

	size_t count = mesh.positions.size();
	vertices.reserve(count);
	shadowVertices.reserve(count);

	for (size_t i = 0; i < count; ++i)
	{
		CSkinnedVertex v{};
		CShadowSkinnedVertex sv{};

		v.position = mesh.positions[i];
		sv.position = mesh.positions[i];

		v.normal = (i < mesh.normals.size()) ? mesh.normals[i] : XMFLOAT3(0, 1, 0);
		v.tex = (i < mesh.texcoords.size()) ? mesh.texcoords[i] : XMFLOAT2(0, 0);

		if (i < mesh.bone_weights.size())
		{
			v.bone_indices = mesh.bone_weights[i].bone_index;
			v.bone_weights = mesh.bone_weights[i].weight;

			// 그림자 정점에도 본 정보는 필수 할당
			sv.bone_indices = mesh.bone_weights[i].bone_index;
			sv.bone_weights = mesh.bone_weights[i].weight;
		}
		else
		{
			v.bone_indices = XMUINT4(0, 0, 0, 0);
			v.bone_weights = XMFLOAT4(1, 0, 0, 0);

			sv.bone_indices = XMUINT4(0, 0, 0, 0);
			sv.bone_weights = XMFLOAT4(1, 0, 0, 0);
		}

		vertices.push_back(v);
		shadowVertices.push_back(sv);
	}

	// 일반 버퍼 세팅
	SetVertices(device, commandList, (UINT)vertices.size(), vertices);

	// 스킨드 그림자 버퍼 세팅
	shadow_stride = sizeof(CShadowSkinnedVertex);
	shadow_vertex_buffer = CreateBufferResource(device, commandList, shadowVertices.data(), shadow_stride * (UINT)shadowVertices.size(), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, shadow_vertex_upload_buffer.GetAddressOf());

	shadow_vertex_buffer_view.BufferLocation = shadow_vertex_buffer->GetGPUVirtualAddress();
	shadow_vertex_buffer_view.StrideInBytes = shadow_stride;
	shadow_vertex_buffer_view.SizeInBytes = shadow_stride * (UINT)shadowVertices.size();
}

// CTriangleMesh
CTriangleMesh::CTriangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	: CMesh(device, commandList)
{
	vertex_num = 3;
	stride = sizeof(CVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CVertex vertices[] = {
		CVertex(XMFLOAT3(0.0f, 1.0f, 0.0f)),
		CVertex(XMFLOAT3(1.0f, -1.0f, 0.0f)),
		CVertex(XMFLOAT3(-1.0f, -1.0f, 0.0f))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());
	
	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

// CRectangleMesh
CRectangleMesh::CRectangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	: CMesh(device, commandList)
{
	// 정점 버퍼 생성
	vertex_num = 4;
	stride = sizeof(CMatVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CMatVertex vertices[] = {
		CMatVertex(XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)),
		CMatVertex(XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f)),
		CMatVertex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f)),
		CMatVertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;

	// 인덱스 버퍼 생성
	index_num = 6;
	submeshes.push_back({ 0, index_num, 0 });
	UINT indexes[] = {
		0,1,3,
		1,2,3
	};

	index_buffer = CreateBufferResource(device, commandList, indexes, sizeof(UINT) * index_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, vertex_upload_buffer.GetAddressOf());

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
	index_buffer_view.SizeInBytes = sizeof(UINT) * index_num;
}

CRectangleMesh::CRectangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float height)
	: CMesh(device, commandList)
{
	// 정점 버퍼 생성
	vertex_num = 4;
	stride = sizeof(CMatVertex);
	//primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;

	CMatVertex vertices[] = { 
		CMatVertex(XMFLOAT3(-halfWidth,  halfHeight, 0.0f), XMFLOAT2(0.0f, 1.0f)),
		CMatVertex(XMFLOAT3(halfWidth,   halfHeight, 0.0f), XMFLOAT2(1.0f, 1.0f)),
		CMatVertex(XMFLOAT3(halfWidth,  -halfHeight, 0.0f), XMFLOAT2(1.0f, 0.0f)),
		CMatVertex(XMFLOAT3(-halfWidth, -halfHeight, 0.0f), XMFLOAT2(0.0f, 0.0f))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;

	// 인덱스 버퍼 생성
	index_num = 6;
	submeshes.push_back({ 0, index_num, 0 });
	UINT indexes[] = {
		0,1,3,
		1,2,3
	};

	index_buffer = CreateBufferResource(device, commandList, indexes, sizeof(UINT) * index_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, vertex_upload_buffer.GetAddressOf());

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
	index_buffer_view.SizeInBytes = sizeof(UINT) * index_num;
}


CBillboardMesh::CBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	: CMesh(device, commandList)
{
	// 정점 버퍼 생성
	vertex_num = 1;
	stride = sizeof(CBillBoardVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	XMFLOAT2 size{ 3.0f, 3.0f };
	CBillBoardVertex vertex{ XMFLOAT3(0.0f, 0.0f, 0.0f) };

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, &vertex, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

CBillboardMesh::CBillboardMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float height)
	: CMesh(device, commandList)
{
	// 정점 버퍼 생성
	vertex_num = 1;
	stride = sizeof(CBillBoardVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	XMFLOAT2 size{ width, height};
	CBillBoardVertex vertex{ XMFLOAT3(0.0f, 0.0f, 0.0f) };

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, &vertex, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

CCubeMesh::CCubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float width, float height, float depth) :
	CMesh(device, commandList)
{
	const size_t vertexSize = 36;

	vertex_num = vertexSize;
	stride = sizeof(CMatVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	float x = width * 0.5f, y = height * 0.5f, z = depth * 0.5f;

	CMatVertex vertices[vertexSize];
	int i{};

	// ⓐ 앞면(Front)
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, -z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, -z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, -z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, -z), XMFLOAT2(0.0f, 1.0f));

	// ⓒ 윗면(Top)
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, +z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, -z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, -z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, -z), XMFLOAT2(0.0f, 1.0f));

	// ⓔ 뒷면(Back)
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, +z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, +z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, +z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, +z), XMFLOAT2(0.0f, 1.0f));

	// ⓖ 아래면(Bottom)
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, -z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, +z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, +z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, +z), XMFLOAT2(0.0f, 1.0f));

	// ⓘ 왼쪽면(Left)
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, -z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, -z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(-x, +y, +z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, -z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(-x, -y, +z), XMFLOAT2(0.0f, 1.0f));

	// ⓚ 오른쪽면(Right)
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, +z), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, +z), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CMatVertex(XMFLOAT3(+x, +y, -z), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, +z), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CMatVertex(XMFLOAT3(+x, -y, -z), XMFLOAT2(0.0f, 1.0f));


	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

CCubeMesh::CCubeMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const XMFLOAT3& halfSize, const XMFLOAT3& pivot)
	: CMesh(device, commandList)
{
	stride = sizeof(CVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	float x = halfSize.x;
	float y = halfSize.y;
	float z = halfSize.z;

	XMFLOAT3 p[8] =
	{
		{-x, -y, -z},
		{-x, +y, -z},
		{+x, +y, -z},
		{+x, -y, -z},
		{-x, -y, +z},
		{-x, +y, +z},
		{+x, +y, +z},
		{+x, -y, +z}
	};

	std::vector<CVertex> vertices =
	{
		CVertex(p[0]), CVertex(p[1]),
		CVertex(p[1]), CVertex(p[2]),
		CVertex(p[2]), CVertex(p[3]),
		CVertex(p[3]), CVertex(p[0]),

		CVertex(p[4]), CVertex(p[5]),
		CVertex(p[5]), CVertex(p[6]),
		CVertex(p[6]), CVertex(p[7]),
		CVertex(p[7]), CVertex(p[4]),

		CVertex(p[0]), CVertex(p[4]),
		CVertex(p[1]), CVertex(p[5]),
		CVertex(p[2]), CVertex(p[6]),
		CVertex(p[3]), CVertex(p[7])
	};

	// pivot 적용
	for (auto& v : vertices) {
		v.position = Vector3::Add(v.position, pivot);
	}

	vertex_num = (UINT)vertices.size();

	vertex_buffer = CreateBufferResource( device, commandList, vertices.data(), stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf() );

	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

CSphereMesh::CSphereMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, float radius, const XMFLOAT3& pivot, UINT sliceCount, UINT stackCount)
	: CMesh(device, commandList)
{
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	stride = sizeof(CVertex);

	std::vector<CVertex> vertices;
	std::vector<UINT> indices;

	// 위도/경도 기반 구 생성
	for (UINT i = 0; i <= stackCount; i++)
	{
		float phi = XM_PI * i / stackCount; // 0 ~ PI

		for (UINT j = 0; j <= sliceCount; j++)
		{
			float theta = XM_2PI * j / sliceCount; // 0 ~ 2PI

			float x = radius * sinf(phi) * cosf(theta);
			float y = radius * cosf(phi);
			float z = radius * sinf(phi) * sinf(theta);

			vertices.emplace_back(XMFLOAT3(x, y, z));
		}
	}

	// 선형 인덱스 생성
	for (UINT i = 0; i < stackCount; i++)
	{
		for (UINT j = 0; j < sliceCount; j++)
		{
			UINT a = i * (sliceCount + 1) + j;
			UINT b = a + 1;
			UINT c = a + (sliceCount + 1);
			UINT d = c + 1;

			// 위도선
			indices.push_back(a);
			indices.push_back(b);

			// 경도선
			indices.push_back(a);
			indices.push_back(c);
		}
	}

	// pivot 적용
	for (auto& v : vertices) {
		v.position = Vector3::Add(v.position, pivot);
	}

	vertex_num = (UINT)vertices.size();
	index_num = (UINT)indices.size();

	SetVertices(device, commandList, vertex_num, vertices);
	SetIndices(device, commandList, index_num, indices);
}