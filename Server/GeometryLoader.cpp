#include "pch.h"
#include "GeometryLoader.h"

using namespace CGeometryLoader;

MeshCollider CGeometryLoader::LoadMeshCollider(BinaryReader& br)
{
    MeshCollider collider;
    int size{ br.Read<int>() };
    collider.positions.reserve(size);
    collider.normals.reserve(size);

    if (br.FindTag("<Positions>:")) br.ReadVectors<XMFLOAT3>(collider.positions);
    if (br.FindTag("<Normals>:")) br.ReadVectors<XMFLOAT3>(collider.normals);
    if (br.FindTag("<SubMeshes>:")) {
        int subMeshCount = br.Read<UINT>();
        collider.indices.reserve(subMeshCount);
        for (int i = 0; i < subMeshCount; ++i)
        {
            std::string tag;
            br.ReadTag(tag);

            int index = br.Read<int>();
            br.ReadVectors<UINT>(collider.indices);
        }
    }

    return collider;
}


Mesh CGeometryLoader::LoadMesh(BinaryReader& br)
{
    Mesh mesh;

    // mesh 정보 read
    if (br.FindTag("<Bounds>:")) mesh.bounds = br.Read<BoundingBox>();

    return mesh;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadFrame(BinaryReader& br)
{
    if (!br.FindTag("<Frame>:"))
        return nullptr;

    auto node = std::make_unique<FrameNode>();
    node->name = br.ReadName();

    std::string tag;
    while (br.ReadTag(tag)) {
        if (br.IsTag(tag, "<Transform>:")) {
            node->local_matrix = br.Read<XMFLOAT4X4>();
        }
        else if (br.IsTag(tag, "<Mesh>:")) {
            node->mesh = LoadMesh(br);
        }
        else if (br.IsTag(tag, "<BoxCount>:")) {
            int count = br.Read<int>();
            for (int i = 0; i < count; ++i) {
                PrimitiveCollider col;
                col.center = br.Read<XMFLOAT3>();
                col.size = br.Read<XMFLOAT3>();
                col.size = Vector3::ScalarProduct(col.size, 0.5);
                node->box_colliders.push_back(col);
            }
        }
        else if (br.IsTag(tag, "<SphereCount>:")) {
            int count = br.Read<int>();
            for (int i = 0; i < count; ++i) {
                PrimitiveCollider col;
                col.center = br.Read<XMFLOAT3>();
                col.radius = br.Read<float>();
                node->sphere_colliders.push_back(col);
            }
        }
        else if (br.IsTag(tag, "<CapsuleCount>:")) {
            int count = br.Read<int>();
            for (int i = 0; i < count; ++i) {
                PrimitiveCollider col;
                col.center = br.Read<XMFLOAT3>();
                col.radius = br.Read<float>();
                col.height = br.Read<float>();
                col.direction = br.Read<int>();
                node->capsule_colliders.push_back(col);
            }
        }
        else if (br.IsTag(tag, "<MeshColCount>:")) {
            int count = br.Read<int>();
            for (int i = 0; i < count; ++i) {
                node->mesh_colliders.push_back(LoadMeshCollider(br));
            }
        }
        else if (br.IsTag(tag, "<Children>:")) {
            int childCount = br.Read<int>();

            node->childrens.reserve(childCount);

            for (int i = 0; i < childCount; ++i)
                node->childrens.push_back(LoadFrame(br));

            break; // Children 끝나면 Frame 끝일 확률 높음
        }
        else if (br.IsTag(tag, "<Frame>:")) {
            // 다음 Frame 시작 → rewind
            br.Stream().seekg(-(std::streamoff)tag.size(), std::ios::cur);
            break;
        }
    }

    return node;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadGeometry(const std::string& filename)
{
    BinaryReader br(filename);
    if (!br.Good())
        return nullptr;

    return LoadFrame(br);
}
