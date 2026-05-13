#pragma once
#include "GeometryLoader.h"
#include "MeshRenderer.h"

template<typename T>
inline void CMeshComponent::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const std::unique_ptr<CGeometryLoader::FrameNode>& node)
{
	auto mesh = std::make_shared<CMesh>();
	CGeometryLoader::Mesh& meshData{ node->mesh };

	mesh->BuildVertices<T>(device, commandList, node);
	mesh->SetSubMesh(meshData.submeshes);
	mesh->SetIndices(device, commandList, (UINT)meshData.indices.size(), meshData.indices);

	SetMesh(mesh);
}

template<typename T>
inline void CMeshComponent::SetMeshFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, const CGeometryLoader::MeshCollider& meshData)
{
	auto mesh = std::make_shared<CMesh>();

	mesh->BuildVertices<T>(device, commandList, meshData);
	mesh->SetIndices(device, commandList, (UINT)meshData.indices.size(), meshData.indices);
	SetMesh(mesh);
}