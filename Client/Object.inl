#pragma once
#include "GeometryLoader.h"
#include "Object.h"

template<typename T>
inline void CObject::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<FrameNode>& node)
{
	auto mesh = std::make_shared<CMesh>();
	Mesh& meshData{ node->mesh };

	mesh->BuildVertices<T>(device, commandList, node);
	mesh->SetIndices(device, commandList, (UINT)meshData.indices.size(), meshData.indices);
	SetMesh(mesh);
	world_matrix = node->localMatrix;
}
