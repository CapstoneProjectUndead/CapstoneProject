#pragma once

class CMesh;

struct FrameNode
{
	std::string name;

	XMFLOAT4X4 localMatrix{};
	XMFLOAT4X4 worldMatrix{};

	std::shared_ptr<CMesh> mesh;

	std::vector<std::unique_ptr<FrameNode>> children;
};

class BinaryReader {
public:
	explicit BinaryReader(const std::string& filename) {
		file.open(filename, std::ios::binary);
	}

	bool good() const { return file.good(); }

    bool FindTag(const std::string& tag)
    {
        size_t matched = 0;
        char ch;
        while (file.get(ch))
        {
            if (ch == tag[matched])
            {
                matched++;
                if (matched == tag.size())
                    return true;
            }
            else
            {
                matched = (ch == tag[0]) ? 1 : 0;
            }
        }
        return false;
    }

    std::string ReadName()
    {
        std::string name;
        char ch;
        while (file.get(ch))
        {
            if (std::isspace(static_cast<unsigned char>(ch)) || ch == '<')
            {
                if (ch == '<')
                    file.unget();
                break;
            }
            name.push_back(ch);
        }
        return name;
    }

    bool ReadMatrix(DirectX::XMFLOAT4X4& out)
    {
        return static_cast<bool>(file.read(reinterpret_cast<char*>(&out), sizeof(float) * 16));
    }

    // 임시: 사이즈 모르는 바이너리 블록을 “다음 태그 전까지” 통째로 잡는 버전
    std::vector<uint8_t> ReadUntilNextTag()
    {
        std::vector<uint8_t> data;
        std::string buf;
        char ch;

        // '<' 가 나오면 태그일 가능성이 있으니 한 글자 되돌림
        while (file.get(ch))
        {
            if (ch == '<')
            {
                file.unget();
                break;
            }
            data.push_back(static_cast<uint8_t>(ch));
        }

        return data;
    }

    std::ifstream& Stream() { return file; }
private:
	std::ifstream file;
};

class CGeometryLoader {
public:
	static std::unique_ptr<FrameNode> LoadGeometry(const std::string& filename, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
private:
    static std::shared_ptr<CMesh> LoadMesh(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    static std::unique_ptr<FrameNode> LoadFrame(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
};

