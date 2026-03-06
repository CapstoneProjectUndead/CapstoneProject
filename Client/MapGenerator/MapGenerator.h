#pragma once

namespace MapGenerator
{
    enum class EModelType : unsigned char {
        ROAD = 0,      // 빈 공간
        WALL,          // 골목길 벽 (#)
        WAREHOUSE,     // 대형 창고 (W)
        STORE,         // 중형 상점 (M)
        KIOSK,         // 소형 가판대 (S)
        TREE,          // 나무/덤불 (T, t)
        TREASURE,      // 보물 ($)
        BENCH,         // 벤치 (n)
        DOOR,          // 상점 문 (d)
        DEFAULT        // 기타
    };

    struct InstanceData {
        XMFLOAT3 position;
        XMFLOAT3 scale;
        EModelType type{ EModelType::ROAD };
    };

    std::vector<InstanceData> Generate3DMap();

    struct Rect {
        int x, y, w, h;
        bool Intersects(const Rect& other) const {
            return !(x + w + 1 <= other.x || x - 1 >= other.x + other.w ||
                y + h + 1 <= other.y || y - 1 >= other.y + other.h);
        }
    };

    static const int WIDTH = 51;
    static const int HEIGHT = 101;

    // 이제 grid는 enum을 직접 담습니다.
    static EModelType mapGrid[HEIGHT][WIDTH];

    static int dx[] = { 0, 0, -2, 2 };
    static int dy[] = { -2, 2, 0, 0 };

    bool IsValid(int x, int y);
    void CarveMaze(int startX, int startY);
    void CreateOpenSpaces(int numSpaces);
    void PlaceSmallKiosk(int cx, int cy);
    void PlaceMediumStore(int cx, int cy);
    void PlaceLargeWarehouse(int cx, int cy);
    void PlaceParkPlaza(int cx, int cy);
}