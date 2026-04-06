#pragma once
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

namespace MapGenerator
{
    enum class EModelType : unsigned char {
        // Road Types
        ROAD = 0,      // 공원 구역의 일반 길
        PARK_GREEN,         // 공원 구역의 수풀/벤치 아래 바닥
        VILLAGE_ROAD,       // 마을(상점) 구역의 일반 길

        WALL,

        //  Buildings
        WAREHOUSE, STORE,
        DOOR,
        CORNER_DOOR,    // HOUSE_WALL_CORNER과 방향 같음

        // Building Wall Types (WareHouse, Store 공용)
        HOUSE_INNTER,    // 건물 내부 바닥
        HOUSE_WALL_STRAIGHT, // 일자 벽
        HOUSE_WALL_CORNER,   // 모서리 벽
        HOUSE_WALL_EMPTY,    // 벽X

        // Props
        KIOSK, TREE, TREASURE, BENCH, SMALL_BUSH, SEESAW,

        // Monster spawn markers (서버 전용 — 클라 렌더링 없음)
        MONSTER_HUMAN,

        UNKNOWN
    };
    enum class ELayer : int {
        FLOOR = 0,   // 바닥 (ROAD, PARK_GREEN, VILLAGE_ROAD, HOUSE_INNER)
        OBJECT = 1,  // 소품 (TREASURE, BENCH, TREE, KIOSK)
        STRUCTURE = 2, // 구조물 (WALL, HOUSE_WALL, DOOR)
        COUNT = 3
    };

    struct Cell { int x, y; };

    struct InstanceData {
        XMFLOAT3 position{};
        float rotationY{ 0.0f }; // scale 대신 회전(도 단위) 추가
        EModelType type{ EModelType::ROAD };

        // 멀티 환경에서만 필요한 변수
        EModelVariant model;

        InstanceData() = default;

        InstanceData(const NetPacket::InstanceData& data)
            : position(data.position)
            , rotationY(data.rotationY)
            , type(static_cast<EModelType>(data.type))
            , model(data.model)
        { }
    };

    std::vector<InstanceData> Generate3DMap();

    struct Rect {
        int x, y, w, h;
        bool Intersects(const Rect& other) const {
            return !(x + w + 1 <= other.x || x - 1 >= other.x + other.w ||
                y + h + 1 <= other.y || y - 1 >= other.y + other.h);
        }
    };

    static const char HOUSE_SIZE{2};

    static const int WIDTH = 51;
    static const int HEIGHT = 101;
    static EModelType mapGrid[(int)ELayer::COUNT][HEIGHT][WIDTH];

    static int dx[] = { 0, 0, -2, 2 };
    static int dy[] = { -2, 2, 0, 0 };

    bool IsValid(int x, int y);
    void RefineBuildingTiles();
    bool IsBuilding(int x, int y);
    bool TryPlaceDoor(int cx, int cy, int size);
    bool IsSpaceForTree(int x, int y);

    void CarveMaze(int startX, int startY);
    void CreateOpenSpaces(int numSpaces);

    void PlaceSmallKiosk(int cx, int cy);
    void PlaceMediumStore(int cx, int cy);
    void PlaceLargeWarehouse(int cx, int cy);
    void PlaceParkPlaza(int cx, int cy);
    void PlaceTreasure();
    void PlaceMonster();
}