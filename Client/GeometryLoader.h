#pragma once

struct FrameNode
{
	std::string name;
	XMFLOAT4X4 transform{};

	std::vector<std::unique_ptr<FrameNode>> children;
};

class BinaryReader {
public:
	explicit BinaryReader(const std::string& filename) {
		file.open(filename, std::ios::binary);
	}

	bool good() const { return file.good(); }

	std::string readToken() {
		std::string token;
		file >> token;
		return token;
	}

	// 정수 읽기 (바이너리)
	int readInt() {
		int v;
		file.read(reinterpret_cast<char*>(&v), sizeof(int));
		return v;
	}

	// raw 구조체 읽기
	template<typename T>
	void readRaw(T& out) {
		file.read(reinterpret_cast<char*>(&out), sizeof(T));
	}

	// 배열 읽기
	template<typename T>
	void readArray(std::vector<T>& out, size_t count) {
		out.resize(count);
		file.read(reinterpret_cast<char*>(out.data()), sizeof(T) * count);
	}

private:
	std::ifstream file;
};

class CGeometryLoader {
public:
	static std::unique_ptr<FrameNode> LoadGeometry(const std::string& filename);
private:
	static std::unique_ptr<FrameNode> LoadFrame(BinaryReader& br);
};

