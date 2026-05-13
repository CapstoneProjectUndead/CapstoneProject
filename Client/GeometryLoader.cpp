#include "stdafx.h"
#include "GeometryLoader.h"

using namespace CGeometryLoader;

//std::unordered_map<std::string, AnimationClip> CGeometryLoader::LoadAnimations(const std::string& filename, int boneCount)
//{
//    std::unordered_map<std::string, AnimationClip> animations;
//
//    BinaryReader br(filename);
//    if (!br.Good())
//        return animations;
//
//    std::ifstream& file = br.Stream();
//    std::string tag;
//
//    // <AnimationClipCount>:
//    br.FindTag("<AnimationClipCount>:");
//    int clipCount = 0;
//    file.read((char*)&clipCount, sizeof(int));
//
//    for (int c = 0; c < clipCount; ++c)
//    {
//        br.FindTag("<AnimationClip>:");
//        std::string clipName = br.ReadName();
//
//        br.FindTag("<ClipLength>:");
//        float clipLength = 0;
//        file.read((char*)&clipLength, sizeof(float));
//
//        br.FindTag("<KeyframeCount>:");
//        int keyCount = 0;
//        file.read((char*)&keyCount, sizeof(int));
//
//        AnimationClip clip;
//        clip.bone_animations.resize(boneCount);
//
//        for (int k = 0; k < keyCount; ++k)
//        {
//            br.FindTag("<Keyframe>:");
//            br.FindTag("<Time>:");
//            float time = 0;
//            file.read((char*)&time, sizeof(float));
//
//            // 모든 bone에 대해 TRS 읽기
//            for (int b = 0; b < boneCount; ++b)
//            {
//                br.FindTag("<Bone>:");
//                int boneIndex = 0;
//                file.read((char*)&boneIndex, sizeof(int));
//
//                Keyframe key;
//                key.time_pos = time;
//
//                br.FindTag("<T>:");
//                key.translation = br.Read<XMFLOAT3>();
//
//                br.FindTag("<R>:");
//                key.rotation = br.Read<XMFLOAT4>();
//
//                br.FindTag("<S>:");
//                key.scale = br.Read<XMFLOAT3>();
//
//                clip.bone_animations[boneIndex].key_frames.push_back(key);
//            }
//
//            br.FindTag("</Keyframe>");
//        }
//
//        animations.emplace(clipName, clip);
//    }
//
//    return animations;
//}

std::unordered_map<std::string, AnimationClip> CGeometryLoader::LoadAnimations(const std::string& filename, int boneCount)
{
    std::unordered_map<std::string, AnimationClip> animations;

    BinaryReader br(filename);
    if (!br.Good())
        return animations;

    std::ifstream& file = br.Stream();

    if (!br.FindTag("<AnimationClipCount>:")) return animations;
    int clipCount = br.Read<int>();

    for (int c = 0; c < clipCount; ++c)
    {
        // <AnimationClip>: (이름)
        if (!br.FindTag("<AnimationClip>:")) break;
        std::string clipName = br.ReadName();

        if (!br.FindTag("<ClipLength>:")) break;
        float clipLength = br.Read<float>();

        // <TotalFrames>: (유니티에서 구운 총 프레임 수)
        if (!br.FindTag("<TotalFrames>:")) break;
        int totalFrames = br.Read<int>();
        AnimationClip clip;
        clip.name = clipName;
        clip.clip_length = clipLength;
        clip.total_frames = totalFrames;
        clip.bone_count = (uint32_t)boneCount;

        int totalMatrixCount = totalFrames * boneCount;
        clip.baked_matrices.reserve(totalMatrixCount);
        clip.toRoot_matrices.reserve(totalMatrixCount);

        // 유니티에서 [Final, ToRoot] 한 쌍씩 썼으므로 반복문으로 읽음
        for (int i = 0; i < totalMatrixCount; ++i)
        {
            XMFLOAT4X4 finalMat, toRootMat;
            file.read(reinterpret_cast<char*>(&finalMat), sizeof(XMFLOAT4X4));
            file.read(reinterpret_cast<char*>(&toRootMat), sizeof(XMFLOAT4X4));

            clip.baked_matrices.push_back(finalMat);
            clip.toRoot_matrices.push_back(toRootMat);
        }

        animations.emplace(clipName, std::move(clip));
    }

    return animations;
}

CGeometryLoader::SkeletonData CGeometryLoader::LoadSkeleton(const std::string& filename)
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

    for (int i = 0; i < (int)skel.bone_names.size(); ++i)
    {
        skel.bone_name_to_index[skel.bone_names[i]] = i;
    }

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
    std::string tag;
    br.ReadTag(tag);
    if (!br.IsTag(tag, "<Materials>:"))
        return;

    int materialCount = br.Read<int>();
    materials.resize(materialCount);

    for (int i = 0; i < materialCount; ++i) {
        // <Material>:
        br.ReadTag(tag);
        int matIndex = br.Read<int>();
        MaterialData& mat = materials[matIndex];

        // 다음 태그들을 순차적으로 읽는다
        while (true) {
            std::streampos pos = br.Stream().tellg();
            if (!br.ReadTag(tag))
                break;

            if (br.IsTag(tag, "</Material>"))
                break;
            if (br.IsTag(tag, "</Materials>"))
                return;
            if (br.IsTag(tag, "<AlbedoColor>:"))
                mat.albedoColor = br.Read<XMFLOAT4>();
            else if (br.IsTag(tag, "<EmissiveColor>:"))
                mat.emissiveColor = br.Read<XMFLOAT4>();
            else if (br.IsTag(tag, "<SpecularColor>:"))
                mat.specularColor = br.Read<XMFLOAT4>();
            else if (br.IsTag(tag, "<Glossiness>:"))
                mat.glossiness = br.Read<float>();
            else if (br.IsTag(tag, "<Smoothness>:"))
                mat.smoothness = br.Read<float>();
            else if (br.IsTag(tag, "<Metallic>:"))
                mat.metallic = br.Read<float>();
            else if (br.IsTag(tag, "<SpecularHighlight>:"))
                mat.specularHighlight = br.Read<float>();
            else if (br.IsTag(tag, "<GlossyReflection>:"))
                mat.glossyReflection = br.Read<float>();
            // --- Textures ---
            else if (br.IsTag(tag, "<AlbedoMap>:")) {
                mat.albedoMap = br.ReadName();
                if (mat.albedoMap == "null") mat.albedoMap = "white";
            }
            else if (br.IsTag(tag, "<SpecularMap>:")) {
                mat.specularMap = br.ReadName();
                if (mat.specularMap == "null") mat.specularMap.clear();
            }
            else if (br.IsTag(tag, "<MetallicMap>:")) {
                mat.metallicMap = br.ReadName();
                if (mat.metallicMap == "null") mat.metallicMap.clear();
            }
            else if (br.IsTag(tag, "<NormalMap>:")) {
                mat.normalMap = br.ReadName();
                if (mat.normalMap == "null") mat.normalMap.clear();
            }
            else if (br.IsTag(tag, "<EmissionMap>:")) {
                mat.emissionMap = br.ReadName();
                if (mat.emissionMap == "null") mat.emissionMap.clear();
            }
            else if (br.IsTag(tag, "<DetailAlbedoMap>:")) {
                mat.detailAlbedoMap = br.ReadName();
                if (mat.detailAlbedoMap == "null") mat.detailAlbedoMap.clear();
            }
            else if (br.IsTag(tag, "<DetailNormalMap>:")) {
                mat.detailNormalMap = br.ReadName();
                if (mat.detailNormalMap == "null") mat.detailNormalMap.clear();
            }
        }
    }
}

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

    int size{ br.Read<int>() };
    mesh.positions.reserve(size);
    mesh.normals.reserve(size);
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
