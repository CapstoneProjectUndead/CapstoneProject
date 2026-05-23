#pragma once
//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

namespace MapGenerator
{
    enum class EModelType : unsigned char {
        // Road Types
        ROAD = 0,           // 공원 구역의 일반 길
        PARK_GREEN,         // 공원 구역의 수풀/벤치 아래 바닥
        VILLAGE_ROAD,       // 마을(상점) 구역의 일반 길

        WALL,
        VILLAGE_WALL,
        PARK_WALL,
        FENCE_WOOD_STR,

        GRASS,      
        DECO_STONE,
        PARK_SAND_DECO,

        // Buildings
        WAREHOUSE, STORE,
        DOOR,
        CORNER_DOOR,

        // Building Wall Types
        HOUSE_INNTER,
        HOUSE_WALL_STRAIGHT,
        HOUSE_WALL_CORNER,
        HOUSE_WALL_EMPTY,

        STORE_WALL_CORNER,
        STORE_WALL_EMPTY,

        // Props
        KIOSK, TREE, 
        TREASURE, 
        TREASURE_VILLAGE,
        BENCH, SMALL_BUSH, SEESAW,

        // Monster spawn markers
        MONSTER_HUMAN,
        MONSTER_GHOST,
        MONSTER_DOG,

        // 땅속에 묻혀있는 보물 (렌더 X, 콜라이더 X, 채굴 가능)
        TREASURE_HIDDEN,

        // 복귀 지점 표시용 데코 (1셀, 라운드 시작부터 표시)
        MANHOLE,

        UNKNOWN
    };

    enum class ELayer : int {
        FLOOR = 0,
        OBJECT = 1,
        STRUCTURE = 2,
        COUNT = 3
    };

    struct Cell { int x, y; };

    struct InstanceData {
        XMFLOAT3 position{};
        float rotationY{ 0.0f };
        EModelType type{ EModelType::ROAD };
        EModelVariant model;

        InstanceData() = default;
        InstanceData(const NetPacket::InstanceData& data)
            : position(data.position)
            , rotationY(data.rotationY)
            , type(static_cast<EModelType>(data.type))
            , model(data.model)
        {
        }
    };

    // --- 상수 섹션 ---

    static const char HOUSE_SIZE{ 1 };

    static const int WIDTH = 25;
    static const int HEIGHT = 51;

    // ----------------------------

    static EModelType mapGrid[(int)ELayer::COUNT][HEIGHT][WIDTH];

    static int dx[] = { 0, 0, -2, 2 };
    static int dy[] = { -2, 2, 0, 0 };

    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::vector<InstanceData> Generate3DMap();
    const std::vector<Cell>& GetStoreCenters();
    const XMFLOAT3& GetManholePosition(); // 복귀 지점 = 맨홀 좌표 (Generate3DMap 호출 후 유효)
    bool IsValid(int x, int y);
    bool IsWalkableFloor(int x, int y);
    bool IsBlockedObject(int x, int y);
    std::vector<Cell> FindPath(int sx, int sy, int ex, int ey);
    int GetBuildingMask(int x, int y, bool isHouse);
    void RefineBuildingTiles();
    bool IsHouseBuilding(int x, int y);
    bool IsStoreBuilding(int x, int y);
    bool IsBlockedStructure(int x, int y);
    bool TryPlaceDoor(int cx, int cy, int size);
    bool IsSpaceForTree(int x, int y);
    void ApplyAreaTheme(int halfHeight);
    float CalculateRotation(int x, int y, EModelType type);
    void CarveMaze(int startX, int startY);
    void CreateOpenSpaces(int numSpaces);
    void PlaceStructures(float areaRatio, int halfHeight);
    void PlaceSmallKiosk(int cx, int cy);
    void PlaceMediumStore(int cx, int cy);
    void PlaceLargeWarehouse(int cx, int cy);
    void PlaceParkPlaza(int cx, int cy);
    void PlaceTreasure();
    void PlaceMonster();
    void PlaceManhole();
 

    struct Rect {
        int x, y, w, h;
        bool Intersects(const Rect& other) const {
            // 충돌 판정 여유 공간을 1칸
            return !(x + w + 1 <= other.x || x - 1 >= other.x + other.w ||
                y + h + 1 <= other.y || y - 1 >= other.y + other.h);
        }
    };
}