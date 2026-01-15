#include "stdafx.h"
#include "Mesh.h"
#include "GeometryLoader.h"

void CGeometryLoader::LoadMaterials(BinaryReader& br, std::vector<Material>& out)
{
    if (!br.FindTag("<Materials>:"))
        return;

    std::ifstream& file{ br.Stream() };

    char countChar;
    file.get(countChar);
    int matCount = (unsigned char)countChar;

    for (int i = 0; i < matCount; ++i)
    {
        if (!br.FindTag("<Material>:"))
            break;

        Material m{};
        // TODO: Unity WriteMaterials 포맷에 맞게 읽기
        // 지금은 태그 사이 바이너리 통째로 읽기
        auto raw = br.ReadUntilNextTag();
        //m.LoadFromBinary(raw);

        out.push_back(m);
    }

    br.FindTag("</Materials>");
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
    std::vector<CVertex> vertices(vertexNum);

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
    //node->name = br.ReadName();

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
