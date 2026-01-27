#include "stdafx.h"
#include "Mesh.h"
#include "SkinnedData.h"
#include "GeometryLoader.h"

std::unordered_map<std::string, int> CGeometryLoader::BuildPathToBoneIndex(const SkeletonData& skeleton)
{
    std::unordered_map<std::string, int> map;

    for (int i = 0; i < skeleton.bone_names.size(); i++)
    {
        const std::string& boneName = skeleton.bone_names[i];
        map[boneName] = i;
    }

    return map;
}

std::string CGeometryLoader::ExtractBoneName(const std::string& path)
{
    size_t pos = path.find_last_of('/');
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

std::unordered_map<std::string, AnimationClip> CGeometryLoader::LoadAnimations(const std::string& filename, int boneCount)
{
    std::unordered_map<std::string, AnimationClip> animations;

    BinaryReader br(filename);
    if (!br.good())
        return animations;

    std::ifstream& file = br.Stream();
    std::string tag;

    // <AnimationClipCount>:
    br.FindTag("<AnimationClipCount>:");
    int clipCount = 0;
    file.read((char*)&clipCount, sizeof(int));

    for (int c = 0; c < clipCount; ++c)
    {
        br.FindTag("<AnimationClip>:");
        std::string clipName = br.ReadName();

        br.FindTag("<ClipLength>:");
        float clipLength = 0;
        file.read((char*)&clipLength, sizeof(float));

        br.FindTag("<KeyframeCount>:");
        int keyCount = 0;
        file.read((char*)&keyCount, sizeof(int));

        AnimationClip clip;
        clip.bone_animations.resize(boneCount);

        for (int k = 0; k < keyCount; ++k)
        {
            br.FindTag("<Keyframe>:");
            br.FindTag("<Time>:");
            float time = 0;
            file.read((char*)&time, sizeof(float));

            // 모든 bone에 대해 TRS 읽기
            for (int b = 0; b < boneCount; ++b)
            {
                br.FindTag("<Bone>:");
                int boneIndex = 0;
                file.read((char*)&boneIndex, sizeof(int));

                Keyframe key;
                key.time_pos = time;

                br.FindTag("<T>:");
                br.ReadFloat3(key.translation);

                br.FindTag("<R>:");
                br.ReadFloat4(key.rotation);

                br.FindTag("<S>:");
                br.ReadFloat3(key.scale);

                clip.bone_animations[boneIndex].key_frames.push_back(key);
            }

            br.FindTag("</Keyframe>");
        }

        animations[clipName] = clip;
    }

    return animations;
}

SkeletonData CGeometryLoader::LoadSkeleton(const std::string& filename)
{
    SkeletonData skel{};

    BinaryReader br(filename);
    if (!br.good())
        return skel;

    std::ifstream& file = br.Stream();

    // 1) BoneCount
    int boneCount = 0;
    if (br.FindTag("<BoneCount>:"))
        file.read((char*)&boneCount, sizeof(int));

    skel.bone_names.resize(boneCount);
    skel.parent_index.resize(boneCount);
    skel.inverse_bind_pose.resize(boneCount);

    // 2) BoneName + BoneLocalMatrix (첫 번째 for문)
    for (int i = 0; i < boneCount; i++)
    {
        br.FindTag("<BoneName>:");
        skel.bone_names[i] = br.ReadName();
    }

    // 3) ParentIndex (두 번째 for문)
    for (int i = 0; i < boneCount; i++)
    {
        br.FindTag("<ParentIndex>:");
        file.read((char*)&skel.parent_index[i], sizeof(int));
    }

    // 4) BindPoses (배열)
    if (br.FindTag("<BindPoses>:"))
    {
        int bindCount = 0;
        file.read((char*)&bindCount, sizeof(int));

        // Unity에서 mesh.bindposes.Length == boneCount일 가능성이 높음
        // 혹시 다르면 맞춰서 resize
        if (bindCount != boneCount)
            skel.inverse_bind_pose.resize(bindCount);

        for (int i = 0; i < bindCount; i++) {
            br.ReadMatrix(skel.inverse_bind_pose[i]);
        }
    }

    return skel;
}

std::shared_ptr<CMesh> CGeometryLoader::LoadMesh(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    std::ifstream& file{ br.Stream() };

    auto mesh = std::make_shared<CMesh>();

    UINT vertexNum;
    file.read(reinterpret_cast<char*>(&vertexNum), sizeof(UINT));

    mesh->name = br.ReadName();

    // 3) 임시 버퍼들
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT4> colors;
    std::vector<XMFLOAT3> normals;
    std::vector<UINT> indices;

    std::vector<XMUINT4> boneIndices;
    std::vector<XMFLOAT4> boneWeights;

    // --- Positions ---
    if (br.FindTag("<Positions>:"))
    {
        UINT count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(UINT));

        positions.resize(count);
        if (count > 0)
            file.read(reinterpret_cast<char*>(positions.data()), sizeof(XMFLOAT3) * count);
    }

    // --- Colors ---
    if (br.FindTag("<Colors>:"))
    {
        UINT count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(UINT));

        colors.resize(count);
        if (count > 0)
            file.read(reinterpret_cast<char*>(colors.data()), sizeof(XMFLOAT4) * count);
    }

    // --- Normals ---
    if (br.FindTag("<Normals>:"))
    {
        UINT count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(UINT));

        normals.resize(count);
        if (count > 0)
            file.read(reinterpret_cast<char*>(normals.data()), sizeof(XMFLOAT3) * count);
    }

    if (br.FindTag("<SubMeshes>:"))
    {
        UINT subMeshCount = 0;
        file.read((char*)&subMeshCount, sizeof(UINT));

        // 우리는 일단 첫 번째 SubMesh만 읽는다고 가정
        if (subMeshCount > 0)
        {
            if (br.FindTag("<SubMesh>:"))
            {
                UINT subMeshIndex = 0;
                file.read((char*)&subMeshIndex, sizeof(UINT));  // Unity에서 WriteIntegers의 n

                UINT indexCount = 0;
                file.read((char*)&indexCount, sizeof(UINT));    // 인덱스 개수

                indices.resize(indexCount);
                if (indexCount > 0)
                    file.read((char*)indices.data(), sizeof(UINT) * indexCount);
            }
        }
    }

    bool Skinned{};
    if (br.FindTag("<BoneWeightCount>:"))
    {
        Skinned = true;
        UINT count = 0;
        file.read((char*)&count, sizeof(UINT));

        boneIndices.resize(count);
        boneWeights.resize(count);

        for (UINT i = 0; i < count; i++)
        {
            // <BoneIndex>:
            br.FindTag("<BoneIndex>:");
            int len = 0;
            file.read((char*)&len, sizeof(int));

            file.read((char*)&boneIndices[i], sizeof(UINT) * len);

            // <BoneWeight>:
            br.FindTag("<BoneWeight>:");
            file.read((char*)&len, sizeof(int));
            file.read((char*)&boneWeights[i], sizeof(float) * len);
        }
    }

    // 최종 정점 배열 조립
    if (Skinned)
    {
        std::vector<CMatVertex> vertices(vertexNum);

        for (UINT i = 0; i < vertexNum; ++i)
        {
            vertices[i].position = (i < positions.size()) ? positions[i] : XMFLOAT3(0, 0, 0);
            vertices[i].color = (i < colors.size()) ? colors[i] : XMFLOAT4(1, 1, 1, 1);
            vertices[i].normal = (i < normals.size()) ? normals[i] : XMFLOAT3(0, 1, 0);

            vertices[i].bone_indices = (i < boneIndices.size()) ? boneIndices[i] : XMUINT4{};
            vertices[i].bone_weights = (i < boneWeights.size()) ? boneWeights[i] : XMFLOAT4(1, 0, 0, 0);
        }

        mesh->SetVertices(device, commandList, vertexNum, vertices);
    }
    else
    {
        std::vector<CVertex> vertices(vertexNum);

        for (UINT i = 0; i < vertexNum; ++i)
        {
            vertices[i].position = (i < positions.size()) ? positions[i] : XMFLOAT3(0, 0, 0);
            vertices[i].color = (i < colors.size()) ? colors[i] : XMFLOAT4(1, 1, 1, 1);
            vertices[i].normal = (i < normals.size()) ? normals[i] : XMFLOAT3(0, 1, 0);
        }

        mesh->SetVertices(device, commandList, vertexNum, vertices);
    }
    
    if (!indices.empty())
        mesh->SetIndices(device, commandList, indices.size(), indices);

    return mesh;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadFrame(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!br.FindTag("<Frame>:"))
        return nullptr;

    auto node = std::make_unique<FrameNode>();

    // Frame 이름
    node->name = br.ReadName();

    // Transform
    br.FindTag("<TransformMatrix>:");
    br.ReadMatrix(node->localMatrix);

    // Mesh 여러 개 있을 수 있음
    while (br.FindTag("<Mesh>:"))
    {
        auto mesh = LoadMesh(br, device, commandList);
        if (mesh)
            node->meshes.push_back(mesh);
    }

    // Children
    int childCount = 0;
    if (br.FindTag("<Children>:"))
    {
        br.Stream().read((char*)&childCount, sizeof(int));

        for (int i = 0; i < childCount; i++)
        {
            auto child = LoadFrame(br, device, commandList);
            if (child)
                node->children.push_back(std::move(child));
        }
    }

    br.FindTag("</Frame>");
    return node;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadGeometry(const std::string& filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    BinaryReader br(filename);
    if (!br.good())
        return nullptr;

    return LoadFrame(br, device, commandList);
}
