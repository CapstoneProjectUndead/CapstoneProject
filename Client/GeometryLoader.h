#pragma once

class CMesh;

struct Keyframe
{
    float time;
    float value;
};

struct Curve
{
    std::string path;        // bone path
    std::string property;    // m_LocalPosition.x 등
    std::vector<Keyframe> keys;
};

struct AnimationClip
{
    std::string name;
    float length = 0.f;
    std::vector<Curve> curves;
};

struct AnimationData
{
    std::vector<AnimationClip> clips;
};

struct FrameNode
{
	std::string name;

	XMFLOAT4X4 localMatrix{};
	XMFLOAT4X4 worldMatrix{};

	std::shared_ptr<CMesh> mesh;

	std::vector<std::unique_ptr<FrameNode>> children;
    AnimationData animation;
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
    static AnimationData LoadAnimations(const std::string& filename);
private:
    static std::shared_ptr<CMesh> LoadMesh(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
    static std::unique_ptr<FrameNode> LoadFrame(BinaryReader& br, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);
};

