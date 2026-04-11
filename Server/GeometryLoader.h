#pragma once

// 서버는 Collider/위치 정보만 load

class BinaryReader {
public:
    explicit BinaryReader(const std::string& filename) {
        file.open(filename, std::ios::binary);
    }

    bool Good() const { return file.good(); }

    bool ReadTag(std::string& outTag)
    {
        outTag.clear();
        char ch;

        // 1. '<' 나올 때까지 무조건 스킵
        while (file.get(ch))
        {
            if (ch == '<')
                break;
        }

        if (!file)
            return false;

        outTag.push_back('<');

        bool sawCloseBracket = false;

        // 2. ':' 나올 때까지 읽기
        std::streampos pos = file.tellg();
        while (file.get(ch))
        {
            // 제어문자 제거 (\0, \r, \b 등)
            if ((unsigned char)ch < 0x20)
                continue;

            outTag.push_back(ch);

            if (ch == '>')
                sawCloseBracket = true;

            // ':'는 태그의 진짜 끝
            if (ch == ':' && sawCloseBracket)
                break;
            else if (ch == '<' && sawCloseBracket) {
                file.seekg(pos);
                break;
            }
        }

        return true;
    }

    inline bool IsTag(const std::string& tag, const char* expected)
    {
        return tag.find(expected) != std::string::npos;
    }

    // tag 나올 때까지 읽기
    bool FindTag(const std::string& tag)
    {
        size_t matched = 0;
        char ch;
        while (file.get(ch)) {
            if (ch == tag[matched]) {
                matched++;
                if (matched == tag.size())
                    return true; // 찾았으면 그대로 true
            }
            else {
                matched = (ch == tag[0]) ? 1 : 0;
            }
        }

        return false;
    }

    std::string ReadName()
    {
        std::string s;
        char ch;

        // 문자열 시작 위치로 이동
        SkipToStringStart();

        // 본문 읽기
        while (file.get(ch)) {
            if (ch == '<') {
                file.unget();
                break;
            }
            if (std::isalnum((unsigned char)ch) || ch == '_' || ch == '/')
                s.push_back(ch);
        }

        return s;
    }

    void SkipToStringStart()
    {
        char ch;

        // 태그 뒤의 ':' 또는 공백 또는 쓰레기 바이트 제거
        while (file.get(ch))
        {
            if (ch == '<') { file.unget(); return; }

            // 문자열의 첫 글자는 알파벳/숫자/언더바
            if (std::isalnum((unsigned char)ch) || ch == '_')
            {
                file.unget(); // 문자열 첫 글자 되돌리기
                return;
            }
        }
    }

    template<typename T>
    T Read()
    {
        T v;
        file.read(reinterpret_cast<char*>(&v), sizeof(T));
        return v;
    }

    template<typename T>
    void ReadVectors(std::vector<T>& out)
    {
        int count = Read<int>();
        out.resize(count);
        for (int i = 0; i < count; ++i)
            out[i] = Read<T>();
    }

    std::ifstream& Stream() { return file; }
private:
    std::ifstream file;
};

namespace CGeometryLoader {
    struct PrimitiveCollider {
        XMFLOAT3 center{};
        XMFLOAT3 size{};       // Box용
        float radius{};        // Sphere/Capsule용
        float height{};        // Capsule용
        int direction{};       // Capsule용
    };

    struct MeshCollider
    {
        std::vector<XMFLOAT3> positions;
        std::vector<XMFLOAT3> normals;
        std::vector<UINT> indices;
    };

    struct Mesh
    {
        BoundingBox bounds;
    };

    // 메쉬가 여러 개면 childrens 사용
    struct FrameNode
    {
        std::string name;
        XMFLOAT4X4 local_matrix;
        Mesh mesh;
        std::vector<MeshCollider> mesh_colliders;
        std::vector<PrimitiveCollider> box_colliders;
        std::vector<PrimitiveCollider> sphere_colliders;
        std::vector<PrimitiveCollider> capsule_colliders;
        std::vector<std::unique_ptr<FrameNode>> childrens;
    };

    // load model
    std::unique_ptr<FrameNode> LoadGeometry(const std::string& filename);
    Mesh LoadMesh(BinaryReader& br);
    MeshCollider LoadMeshCollider(BinaryReader& br);
    std::unique_ptr<FrameNode> LoadFrame(BinaryReader& br);
};