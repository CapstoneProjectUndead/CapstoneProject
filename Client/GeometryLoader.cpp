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

                /*br.FindTag("<T>:");
                br.Read(key.translation);

                br.FindTag("<R>:");
                br.Read(key.rotation);

                br.FindTag("<S>:");
                br.Read(key.scale);*/

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
    //SkeletonData skel{};

    //BinaryReader br(filename);
    //if (!br.Good())
    //    return skel;

    //std::ifstream& file = br.Stream();

    //// 1) BoneCount
    //int boneCount = 0;
    //if (br.FindTag("<BoneCount>:"))
    //    file.read((char*)&boneCount, sizeof(int));

    //skel.bone_names.resize(boneCount);
    //skel.parent_index.resize(boneCount);
    //skel.inverse_bind_pose.resize(boneCount);

    //// 2) BoneName + BoneLocalMatrix (첫 번째 for문)
    //for (int i = 0; i < boneCount; i++)
    //{
    //    br.FindTag("<BoneName>:");
    //    skel.bone_names[i] = br.ReadName();
    //}

    //// 3) ParentIndex (두 번째 for문)
    //for (int i = 0; i < boneCount; i++)
    //{
    //    br.FindTag("<ParentIndex>:");
    //    file.read((char*)&skel.parent_index[i], sizeof(int));
    //}

    //// 4) BindPoses (배열)
    //if (br.FindTag("<BindPoses>:"))
    //{
    //    int bindCount = 0;
    //    file.read((char*)&bindCount, sizeof(int));

    //    // Unity에서 mesh.bindposes.Length == boneCount일 가능성이 높음
    //    // 혹시 다르면 맞춰서 resize
    //    if (bindCount != boneCount)
    //        skel.inverse_bind_pose.resize(bindCount);

    //    for (int i = 0; i < bindCount; i++) {
    //        skel.inverse_bind_pose[i] = br.Read();
    //    }
    //}

    //return skel;
    return SkeletonData{};
}

Mesh CGeometryLoader::LoadMesh(BinaryReader& br)
{
    Mesh mesh;
    std::string tag;

    mesh.positions.reserve(br.Read<int>());
    mesh.normals.reserve(br.Read<int>());

    // mesh 정보 read
    if (br.FindTag("<Positions>:")) br.ReadVector<XMFLOAT3>(mesh.positions);
    if(br.FindTag("<Normals>:")) br.ReadVector<XMFLOAT3>(mesh.normals);
    if (br.FindTag("<SubMeshes>:")) {
        int subMeshCount = br.Read<UINT>();
        mesh.indices.reserve(subMeshCount);
        for (int i = 0; i < subMeshCount; ++i)
        {
            std::string tag;
            br.ReadTag(tag);
            
            int index = br.Read<int>();
            br.ReadVector<UINT>(mesh.indices);
        }
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
