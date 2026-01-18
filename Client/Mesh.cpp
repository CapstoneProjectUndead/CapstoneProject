#include "stdafx.h"
#include "Mesh.h"

// CDiffuseVertex
CDiffuseVertex::CDiffuseVertex()
	: CVertex(), tex{}
{
}

CDiffuseVertex::CDiffuseVertex(XMFLOAT3 position, XMFLOAT4 color, XMFLOAT2 tex)
	: CVertex(position, color), tex{ tex }
{
}

CBillBoardVertex::CBillBoardVertex()
	: size{ 3, 3 }
{
}
CBillBoardVertex::CBillBoardVertex(XMFLOAT3 position, XMFLOAT2 size)
	: position{position} , size{size}
{

}

// CMesh
CMesh::CMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
}

void CMesh::ReleaseUploadBuffer()
{
	if (vertex_upload_buffer) vertex_upload_buffer.Reset();
	vertex_upload_buffer = nullptr;
}

void CMesh::Render(ID3D12GraphicsCommandList* commandList)
{
	// 프리미티브 유형 설정
	commandList->IASetPrimitiveTopology(primitive_topology);
	// 정점 버퍼 뷰 설정
	commandList->IASetVertexBuffers(slot_num, 1, &vertex_buffer_view);
	if (index_buffer) {
		commandList->IASetIndexBuffer(&index_buffer_view);
		commandList->DrawIndexedInstanced(index_num, 1, start_index, base_vertex_index, 0);
	}
	else {
		// 렌더링(입력 조립기 작동)
		commandList->DrawInstanced(vertex_num, 1, offset, 0);
	}
}

void CMesh::SetVertices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<CVertex> vertices)
{
	vertex_num = num;
	stride = sizeof(CVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices.data(), stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}

void CMesh::SetIndices(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, UINT num, std::vector<UINT> indices)
{
	index_num = num;

	index_buffer = CreateBufferResource(device, commandList, indices.data(), sizeof(UINT) * index_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, index_upload_buffer.GetAddressOf());

	index_buffer_view.BufferLocation = index_buffer->GetGPUVirtualAddress();
	index_buffer_view.Format = DXGI_FORMAT_R32_UINT;
	index_buffer_view.SizeInBytes = sizeof(UINT) * index_num;
}

// CTriangleMesh
CTriangleMesh::CTriangleMesh(ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
	: CMesh(device, commandList)
{
	vertex_num = 3;
	stride = sizeof(CVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CVertex vertices[] = {
		CVertex(XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),
		CVertex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f)),
		CVertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Blue))
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
	stride = sizeof(CDiffuseVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CDiffuseVertex vertices[] = {
		CDiffuseVertex(XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f)),
		CDiffuseVertex(XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT4(0.0f,1.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)),
		CDiffuseVertex(XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(1.0f, 1.0f)),
		CDiffuseVertex(XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT4(Colors::Blue), XMFLOAT2(0.0f, 1.0f))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;

	// 인덱스 버퍼 생성
	index_num = 6;
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
	stride = sizeof(CDiffuseVertex);
	//primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;

	CDiffuseVertex vertices[] = {
		CDiffuseVertex(XMFLOAT3(-halfWidth,  halfHeight, 0.0f), XMFLOAT4(1, 0, 0, 1), XMFLOAT2(0.0f, 0.0f)),
		CDiffuseVertex(XMFLOAT3(halfWidth,  halfHeight, 0.0f), XMFLOAT4(0, 1, 0, 1), XMFLOAT2(1.0f, 0.0f)),
		CDiffuseVertex(XMFLOAT3(halfWidth, -halfHeight, 0.0f), XMFLOAT4(0, 0, 1, 1), XMFLOAT2(1.0f, 1.0f)),
		CDiffuseVertex(XMFLOAT3(-halfWidth, -halfHeight, 0.0f), XMFLOAT4(0, 0, 1, 1), XMFLOAT2(0.0f, 1.0f))
	};

	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;

	// 인덱스 버퍼 생성
	index_num = 6;
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
	CBillBoardVertex vertex{ XMFLOAT3(0.0f, 0.0f, 0.0f), size };

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
	CBillBoardVertex vertex{ XMFLOAT3(0.0f, 0.0f, 0.0f), size };

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
	stride = sizeof(CDiffuseVertex);
	primitive_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	float x = width * 0.5f, y = height * 0.5f, z = depth * 0.5f;

	CDiffuseVertex vertices[vertexSize];
	int i{};

	// ⓐ 앞면(Front)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, -z), XMFLOAT4(Colors::Coral), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, -z), XMFLOAT4(Colors::Orange), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, -z), XMFLOAT4(Colors::Gold), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, -z), XMFLOAT4(Colors::Coral), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, -z), XMFLOAT4(Colors::Gold), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, -z), XMFLOAT4(Colors::Tomato), XMFLOAT2(0.0f, 1.0f));

	// ⓒ 윗면(Top)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, +z), XMFLOAT4(Colors::SkyBlue), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, +z), XMFLOAT4(Colors::LightBlue), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, -z), XMFLOAT4(Colors::SteelBlue), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, +z), XMFLOAT4(Colors::SkyBlue), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, -z), XMFLOAT4(Colors::SteelBlue), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, -z), XMFLOAT4(Colors::DodgerBlue), XMFLOAT2(0.0f, 1.0f));

	// ⓔ 뒷면(Back)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, +z), XMFLOAT4(Colors::MediumPurple), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, +z), XMFLOAT4(Colors::Orchid), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, +z), XMFLOAT4(Colors::Plum), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, +z), XMFLOAT4(Colors::MediumPurple), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, +z), XMFLOAT4(Colors::Plum), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, +z), XMFLOAT4(Colors::Thistle), XMFLOAT2(0.0f, 1.0f));

	// ⓖ 아래면(Bottom)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, -z), XMFLOAT4(Colors::DarkGreen), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, -z), XMFLOAT4(Colors::ForestGreen), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, +z), XMFLOAT4(Colors::SeaGreen), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, -z), XMFLOAT4(Colors::DarkGreen), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, +z), XMFLOAT4(Colors::SeaGreen), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, +z), XMFLOAT4(Colors::LimeGreen), XMFLOAT2(0.0f, 1.0f));

	// ⓘ 왼쪽면(Left)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, +z), XMFLOAT4(Colors::Sienna), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, -z), XMFLOAT4(Colors::SaddleBrown), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, -z), XMFLOAT4(Colors::Chocolate), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, +y, +z), XMFLOAT4(Colors::Sienna), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, -z), XMFLOAT4(Colors::Chocolate), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(-x, -y, +z), XMFLOAT4(Colors::Peru), XMFLOAT2(0.0f, 1.0f));

	// ⓚ 오른쪽면(Right)
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, -z), XMFLOAT4(Colors::SlateGray), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, +z), XMFLOAT4(Colors::LightSlateGray), XMFLOAT2(1.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, +z), XMFLOAT4(Colors::DarkSlateGray), XMFLOAT2(1.0f, 1.0f));

	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, +y, -z), XMFLOAT4(Colors::SlateGray), XMFLOAT2(0.0f, 0.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, +z), XMFLOAT4(Colors::DarkSlateGray), XMFLOAT2(1.0f, 1.0f));
	vertices[i++] = CDiffuseVertex(XMFLOAT3(+x, -y, -z), XMFLOAT4(Colors::DimGray), XMFLOAT2(0.0f, 1.0f));


	// 삼각형 메쉬를 리소스로 생성
	vertex_buffer = CreateBufferResource(device, commandList, vertices, stride * vertex_num, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, vertex_upload_buffer.GetAddressOf());

	// 정점 버퍼 뷰 설정
	vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
	vertex_buffer_view.StrideInBytes = stride;
	vertex_buffer_view.SizeInBytes = stride * vertex_num;
}
