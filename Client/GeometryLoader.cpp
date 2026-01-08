#include "stdafx.h"
#include "GeometryLoader.h"

std::unique_ptr<FrameNode> CGeometryLoader::LoadFrame(BinaryReader& br)
{
    return nullptr;
}

std::unique_ptr<FrameNode> CGeometryLoader::LoadGeometry(const std::string& filename)
{
    BinaryReader br(filename);
    if (!br.good()) return nullptr;

    while (true) {
        std::string token = br.readToken();
        if (token == "<Hierarchy>:")
            return LoadFrame(br);
        if (token == "</Hierarchy>")
            break;
    }
    return nullptr;
}
