#pragma once
#include <vector>
#include <DirectXMath.h>

using namespace DirectX;

// 📦 GPU로 보낼 상자 1개의 데이터 (인스턴스 데이터)
struct InstanceData {
    XMFLOAT3 position; // 3D 위치 (X, Y, Z)
    XMFLOAT3 scale;    // 크기 (가로, 높이, 깊이)
    XMFLOAT4 color;    // 색상 (R, G, B, A)
};

class MapGenerator {
public:
    // 맵을 생성하고, 3D 상자들의 리스트를 반환하는 마법의 함수!
    static std::vector<InstanceData> Generate3DMap();
};