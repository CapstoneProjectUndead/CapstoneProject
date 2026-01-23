#include "stdafx.h"
#include "Mesh.h"
#include "GeometryLoader.h"

AnimationData CGeometryLoader::LoadAnimations(const std::string& filename)
{
    AnimationData animData;

    BinaryReader br(filename);
    if (!br.good())
        return animData;

    std::ifstream& file = br.Stream();
    
    int clipCount = 0;
    if (br.FindTag("<AnimationClipCount>:")) {
        file.read((char*)&clipCount, sizeof(int));
    }

    for (int c = 0; c < clipCount; c++)
    {
        AnimationClip clip;

        // <AnimationClip>:
        br.FindTag("<AnimationClip>:");
        clip.name = br.ReadName();

        // <ClipLength>:
         br.FindTag("<ClipLength>:");
        file.read((char*)&clip.length, sizeof(float));

        // <CurveCount>:
        br.FindTag("<CurveCount>:");
        int curveCount = 0;
        file.read((char*)&curveCount, sizeof(int));

        clip.curves.resize(curveCount);

        for (int i = 0; i < curveCount; i++)
        {
            Curve curve;

            // <CurvePath>:
            br.FindTag("<CurvePath>:");
            curve.path = br.ReadName();

            // <CurveProperty>:
            br.FindTag("<CurveProperty>:");
            curve.property = br.ReadName();

            // <KeyCount>:
            br.FindTag("<KeyCount>:");
            int keyCount = 0;
            file.read((char*)&keyCount, sizeof(int));

            curve.keys.resize(keyCount);

            for (int k = 0; k < keyCount; k++)
            {
                // <KeyTime>:
                br.FindTag("<KeyTime>:");
                file.read((char*)&curve.keys[k].time, sizeof(float));

                // <KeyValue>:
                br.FindTag("<KeyValue>:");
                file.read((char*)&curve.keys[k].value, sizeof(float));
            }

            clip.curves[i] = std::move(curve);
        }

        animData.clips.push_back(std::move(clip));
    }

    return animData;
}

std::shared_ptr<CMesh> CGeometryLoader::LoadMesh(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    if (!br.FindTag("<Mesh>:"))
        return nullptr;

    std::ifstream& file{ br.Stream() };

    auto mesh = std::make_shared<CMesh>();

    UINT vertexNum;
    file.read(reinterpret_cast<char*>(&vertexNum), sizeof(UINT));

    // 3) 임시 버퍼들
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT4> colors;
    std::vector<XMFLOAT3> normals;
    std::vector<UINT> indices;

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

    // </Mesh> 태그 스킵
    br.FindTag("</Mesh>");

    // 최종 정점 배열 조립
    std::vector<CMatVertex> vertices(vertexNum);

    for (UINT i = 0; i < vertexNum; ++i)
    {
        // Position
        if (i < positions.size())
            vertices[i].position = positions[i];
        else
            vertices[i].position = XMFLOAT3(0, 0, 0); // 기본값 (필요시 변경)

        // Color
        if (i < colors.size())
            vertices[i].color = colors[i];
        else
            vertices[i].color = XMFLOAT4(1, 1, 1, 1); // 기본값 (white)

        // Normal
        if (i < normals.size())
            vertices[i].normal = normals[i];
        else
            vertices[i].normal = XMFLOAT3(1, 1, 1); // 기본값 (white)
    }

    // vertex, index gpu에 set
    mesh->SetVertices(device, commandList, vertexNum, vertices);

    if (!indices.empty())
        mesh->SetIndices(device, commandList, indices.size(), indices);

    return mesh;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadFrame(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    // <Frame>: 태그 찾기
    if (!br.FindTag("<Frame>:"))
        return nullptr;

    auto node = std::make_unique<FrameNode>();

    // 이름 읽기
    node->name = br.ReadName();

    std::ifstream& file{ br.Stream() };

    {
        std::streampos pos = file.tellg();
        if (br.FindTag("<Mesh>:"))
        {
            file.clear();
            file.seekg(pos);
            node->mesh = LoadMesh(br, device, commandList);
        }
        else
        {
            file.clear();
            file.seekg(pos);
        }
    }

    return node;

}

std::unique_ptr<FrameNode> CGeometryLoader::LoadGeometry(const std::string& filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    BinaryReader br(filename);
    if (!br.good())
        return nullptr;

    return LoadFrame(br, device, commandList);
}
