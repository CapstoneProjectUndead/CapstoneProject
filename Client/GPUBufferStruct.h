#pragma once
#include "Material.h"

// GPU에 넘겨줄 배열 구조체
struct InstCB {
    XMFLOAT4X4 world_matrix;
    MaterialData material;
};

struct AnimationData {
    uint32_t start_offset; // 클립 시작 위치
    uint32_t cur_frame;         // 현재 프레임
    uint32_t bone_count;        // 캐릭터의 본 개수
};

struct AniCB {
    XMFLOAT4X4 world_matrix;
    MaterialData material;
    AnimationData ani_data;
};

struct UIInstCB {
    XMFLOAT4X4 world_matrix; // UI의 위치, 크기, 회전이 담긴 행렬
    MaterialData material;
};

struct BillboardInstCB {
    XMFLOAT4X4 world_matrix;
    MaterialData material;
};

struct TextInst {
    std::wstring text;
    XMFLOAT4X4 world_matrix;
    XMFLOAT4 color;
    bool is_billboard;
};
