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

    // 이름 읽기
    UINT vertexNum;
    file.read((char*)&vertexNum, sizeof(UINT));

    // 정점 데이터 임시 저장
    std::vector<XMFLOAT3> positions(vertexNum);
    std::vector<XMFLOAT4> colors(vertexNum);
    std::vector<XMFLOAT2> uv0(vertexNum);
    std::vector<XMFLOAT2> uv1(vertexNum);
    std::vector<XMFLOAT3> normals(vertexNum);

    // <Bounds>:
    if (br.FindTag("<Bounds>:"))
    {
        auto boundsRaw = br.ReadUntilNextTag();
        //mesh->LoadBounds(boundsRaw);
    }

    // 3) Positions
    if (br.FindTag("<Positions>:"))
    {
        file.read((char*)positions.data(), sizeof(XMFLOAT3) * vertexNum);
    }

    // 4) Colors
    if (br.FindTag("<Colors>:"))
    {
        file.read((char*)colors.data(), sizeof(XMFLOAT4) * vertexNum);
    }

    // 5) UV0
    if (br.FindTag("<TextureCoords0>:"))
    {
        br.Stream().read((char*)uv0.data(), sizeof(XMFLOAT2) * vertexNum);
    }


    // </Mesh> 태그 스킵
    br.FindTag("</Mesh>");

    std::vector<CDiffuseVertex> vertices(vertexNum);

    for (int i = 0; i < vertexNum; ++i)
    {
        XMFLOAT3 pos = positions.empty() ? XMFLOAT3(0, 0, 0) : positions[i];
        XMFLOAT4 col = colors.empty() ? XMFLOAT4(1, 1, 1, 1) : colors[i];
        XMFLOAT2 uv = uv0.empty() ? XMFLOAT2(0, 0) : uv0[i];

        vertices[i] = CDiffuseVertex(pos, col, uv);
    }

    mesh->SetVertices(device, commandList, vertexNum, vertices);

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
