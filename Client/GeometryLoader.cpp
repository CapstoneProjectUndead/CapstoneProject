#include "stdafx.h"
#include "Mesh.h"
#include "SkinnedData.h"
#include "GeometryLoader.h"

std::unordered_map<std::string, AnimationClip> CGeometryLoader::LoadAnimations(const std::string& filename, int boneCount)
{
    std::unordered_map<std::string, AnimationClip> animations;

    BinaryReader br(filename);
    if (!br.Good())
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
                key.translation = br.Read<XMFLOAT3>();

                br.FindTag("<R>:");
                key.rotation = br.Read<XMFLOAT4>();

                br.FindTag("<S>:");
                key.scale = br.Read<XMFLOAT3>();

                clip.bone_animations[boneIndex].key_frames.push_back(key);
            }

            br.FindTag("</Keyframe>");
        }

        animations.emplace(clipName, clip);
    }

    return animations;
}

SkeletonData CGeometryLoader::LoadSkeleton(const std::string& filename)
{
    SkeletonData skel{};

    BinaryReader br(filename);
    if (!br.Good())
        return skel;

    // 1) BoneCount
    br.FindTag("<BoneCount>:");
    int boneCount = br.Read<int>();

    skel.bone_names.resize(boneCount);
    skel.parent_index.resize(boneCount);
    skel.inverse_bind_pose.resize(boneCount);

    // 2) BoneName + BoneLocalMatrix
    for (int i = 0; i < boneCount; ++i) {
        // <BoneName>:
        br.FindTag("<BoneName>:");
        skel.bone_names[i] = br.ReadName();

        // <BoneLocalMatrix>:
        //br.FindTag("<BoneLocalMatrix>:");
    }

    // 3) ParentIndex
    for (int i = 0; i < boneCount; ++i)
    {
        br.FindTag("<ParentIndex>:");
        skel.parent_index[i] = br.Read<int>();
    }

    // 4) BindPoses (inverse bind pose)
    br.FindTag("<BindPoses>:");
    {
        int count = br.Read<int>();
        for (int i = 0; i < count; ++i)
        {
            skel.inverse_bind_pose[i] = br.Read<XMFLOAT4X4>();
        }
    }
    br.FindTag("</Skeleton>");

    return skel;
}

void CGeometryLoader::LoadBoneWeights(BinaryReader& br, Mesh& mesh)
{
    if (!br.FindTag("<BoneWeightCount>:"))
        return;

    int count = br.Read<int>();
    mesh.bone_weights.resize(count);

    for (int i = 0; i < count; ++i)
    {
        // <BoneIndex>:
        br.FindTag("<BoneIndex>:");
        int len = br.Read<int>();
        mesh.bone_weights[i].bone_index = br.Read<XMUINT4>();

        // <BoneWeight>:
        br.FindTag("<BoneWeight>:");
        len = br.Read<int>();
        mesh.bone_weights[i].weight = br.Read<XMFLOAT4>();
    }
}

void CGeometryLoader::LoadMaterials(BinaryReader& br, std::vector<MaterialData>& materials)
{
    // <Materials>:
    if (!br.FindTag("<Materials>:"))
        return;

    int materialCount = br.Read<int>();
    materials.resize(materialCount);

    for (int i = 0; i < materialCount; ++i) {
        std::string tag;
        br.ReadTag(tag); // "<Material>:"
        int matIndex = br.Read<int>();

        MaterialData& mat = materials[matIndex];

        // --- Colors ---
        if (br.FindTag("<AlbedoColor>:"))
            mat.albedoColor = br.Read<XMFLOAT4>();

        if (br.FindTag("<EmissiveColor>:"))
            mat.emissiveColor = br.Read<XMFLOAT4>();

        if (br.FindTag("<SpecularColor>:"))
            mat.specularColor = br.Read<XMFLOAT4>();

        // --- Floats ---
        if (br.FindTag("<Glossiness>:"))
            mat.glossiness = br.Read<float>();

        if (br.FindTag("<Smoothness>:"))
            mat.smoothness = br.Read<float>();

        if (br.FindTag("<Metallic>:"))
            mat.metallic = br.Read<float>();

        if (br.FindTag("<SpecularHighlight>:"))
            mat.specularHighlight = br.Read<float>();

        if (br.FindTag("<GlossyReflection>:"))
            mat.glossyReflection = br.Read<float>();

        // --- Textures ---
        if (br.FindTag("<AlbedoMap>:")) {
            mat.albedoMap = br.ReadName();
            if (mat.albedoMap == "null") mat.albedoMap.clear();
        }

        if (br.FindTag("<SpecularMap>:")) {
            mat.specularMap = br.ReadName();
            if (mat.specularMap == "null") mat.specularMap.clear();
        }

        if (br.FindTag("<MetallicMap>:")) {
            mat.metallicMap = br.ReadName();
            if (mat.metallicMap == "null") mat.metallicMap.clear();
        }

        if (br.FindTag("<NormalMap>:")) {
            mat.normalMap = br.ReadName();
            if (mat.normalMap == "null") mat.normalMap.clear();
        }

        if (br.FindTag("<EmissionMap>:")) {
            mat.emissionMap = br.ReadName();
            if (mat.emissionMap == "null") mat.emissionMap.clear();
        }

        if (br.FindTag("<DetailAlbedoMap>:")) {
            mat.detailAlbedoMap = br.ReadName();
            if (mat.detailAlbedoMap == "null") mat.detailAlbedoMap.clear();
        }

        if (br.FindTag("<DetailNormalMap>:")) {
            mat.detailNormalMap = br.ReadName();
            if (mat.detailNormalMap == "null") mat.detailNormalMap.clear();
        }
    }

    // </Materials>
    br.FindTag("</Materials>");
}

Mesh CGeometryLoader::LoadMesh(BinaryReader& br)
{
    Mesh mesh;
    std::string tag;

    mesh.positions.reserve(br.Read<int>());
    mesh.normals.reserve(br.Read<int>());
    int materialCount = br.Read<int>();
    bool skinned = br.Read<bool>();

    // mesh 정보 read
    if (br.FindTag("<Bounds>:")) mesh.bounds = br.Read<BoundingBox>();
    if (br.FindTag("<Positions>:")) br.ReadVectors<XMFLOAT3>(mesh.positions);
    // TextureCoords read
    if (materialCount > 0) {
        if (br.FindTag("<TextureCoords0>:")) br.ReadVectors<XMFLOAT2>(mesh.texcoords);
    }
    if(br.FindTag("<Normals>:")) br.ReadVectors<XMFLOAT3>(mesh.normals);
    if (br.FindTag("<SubMeshes>:")) {
        int subMeshCount = br.Read<UINT>();
        mesh.indices.reserve(subMeshCount);
        for (int i = 0; i < subMeshCount; ++i)
        {
            std::string tag;
            br.ReadTag(tag);
            
            int index = br.Read<int>();
            br.ReadVectors<UINT>(mesh.indices);
        }
    }
    // meterial 정보 read
    if (materialCount > 0) {
        LoadMaterials(br, mesh.materials);
    }
    if (skinned) {
        LoadBoneWeights(br, mesh);
    }

    return mesh;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadFrame(BinaryReader& br)
{
    std::string tag;
    //br.ReadTag(tag);
    if (!br.FindTag("<Frame>:")) {
        return nullptr;
    }

    auto node = std::make_unique<FrameNode>();
    // Name
    node->name = br.ReadName();

    // Transform
    if (br.FindTag("<Transform>:")) {
        node->localMatrix = br.Read<XMFLOAT4X4>();
    }

    br.ReadTag(tag);
    if (br.IsTag(tag, "<Mesh>:"))
    {
        node->mesh = LoadMesh(br);
    }
    else if (br.IsTag(tag, "<Children>:")) {
        int childCount = br.Read<int>();

        node->childrens.reserve(childCount);
        for (int i = 0; i < childCount; ++i)
        {
            node->childrens.push_back(LoadFrame(br));
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
