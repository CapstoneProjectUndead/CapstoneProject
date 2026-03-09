#include "stdafx.h"
#include "MapGenerator.h"
using namespace MapGenerator;

// 특정 레이어의 타일 타입을 안전하게 가져옴
EModelType GetTile(ELayer layer, int x, int y) {
    if (!IsValid(x, y)) return EModelType::UNKNOWN;
    return mapGrid[(int)layer][y][x];
}

// 빌딩 내부 및 외벽 배치 공통 로직
void SetBuildingArea(int cx, int cy, int size, EModelType structType) {
    for (int y = cy - size; y <= cy + size; y++) {
        for (int x = cx - size; x <= cx + size; x++) {
            if (!IsValid(x, y)) continue;
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::HOUSE_INNTER;
            mapGrid[(int)ELayer::STRUCTURE][y][x] = structType;
        }
    }
}

// --- 메인 API 함수 ---
std::vector<InstanceData> MapGenerator::Generate3DMap() {
    srand((unsigned int)time(NULL));
    float areaRatio = (WIDTH * HEIGHT) / 5000.0f;
    int halfHeight = HEIGHT / 2;

    // 1. 초기화 (벽으로 채우기)
    for (int l = 0; l < (int)ELayer::COUNT; l++)
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                mapGrid[l][y][x] = (l == (int)ELayer::FLOOR) ? EModelType::WALL : EModelType::UNKNOWN;

    // 2. 지형 생성 (미로 + 오픈 스페이스)
    CarveMaze(1, 1);
    CreateOpenSpaces(max(5, (int)(20 * areaRatio)));

    // 3. 구역 테마 적용 및 구조물 스폰
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (GetTile(ELayer::FLOOR, x, y) == EModelType::ROAD && y > halfHeight)
                mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
        }
    }

    // 구조물 배치 (반복문 간소화 가능하나 명확성을 위해 유지)
    for (int i = 0; i < max(1, (int)(3 * areaRatio)); i++) PlaceLargeWarehouse(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 10));
    for (int i = 0; i < max(2, (int)(10 * areaRatio)); i++) PlaceMediumStore(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));
    for (int i = 0; i < max(5, (int)(25 * areaRatio)); i++) PlaceSmallKiosk(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));
    for (int i = 0; i < max(5, (int)(20 * areaRatio)); i++) PlaceParkPlaza(3 + rand() % (WIDTH - 6), 5 + rand() % (halfHeight - 10));

    PlaceTreasure();
    RefineBuildingTiles();

    // 4. 인스턴스 데이터 변환
    std::vector<InstanceData> instanceList;
    const float TILE_SIZE = 2.0f;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            for (int l = 0; l < (int)ELayer::COUNT; l++) {
                EModelType type = mapGrid[l][y][x];
                if (type == EModelType::UNKNOWN) continue;

                InstanceData inst;
                inst.type = type;
                inst.position = XMFLOAT3(x * TILE_SIZE, l * 0.01f, y * TILE_SIZE);

                if (l == (int)ELayer::STRUCTURE) {
                    int bMask = 0;
                    if (IsBuilding(x, y - 1)) bMask |= 1; // 상
                    if (IsBuilding(x, y + 1)) bMask |= 2; // 하
                    if (IsBuilding(x - 1, y)) bMask |= 4; // 좌
                    if (IsBuilding(x + 1, y)) bMask |= 8; // 우

                    if (type == EModelType::HOUSE_WALL_STRAIGHT || type == EModelType::DOOR) {
                        if (!(bMask & 1)) inst.rotationY = 0.0f;
                        else if (!(bMask & 8)) inst.rotationY = 270.0f;
                        else if (!(bMask & 2)) inst.rotationY = 180.0f;
                        else if (!(bMask & 4)) inst.rotationY = 90.0f;
                    }
                    else if (type == EModelType::HOUSE_WALL_CORNER || type == EModelType::CORNER_DOOR) {
                        if ((bMask & 10) == 10)      inst.rotationY = 0.0f;
                        else if ((bMask & 6) == 6)   inst.rotationY = 270.0f;
                        else if ((bMask & 5) == 5)   inst.rotationY = 180.0f;
                        else if ((bMask & 9) == 9)   inst.rotationY = 90.0f;
                    }
                }
                instanceList.push_back(inst);
            }
        }
    }

    std::sort(instanceList.begin(), instanceList.end(), [](const InstanceData& a, const InstanceData& b) {
        return (int)a.type < (int)b.type;
        });

    return instanceList;
}

// --- 로직 함수들 ---
bool MapGenerator::TryPlaceDoor(int cx, int cy, int size) {
    struct DoorPos { int x, y; bool isCorner; };
    std::vector<DoorPos> candidates = {
        {cx, cy - size, false}, {cx, cy + size, false}, {cx - size, cy, false}, {cx + size, cy, false}, // 중앙
        {cx - size, cy - size, true}, {cx + size, cy - size, true}, {cx - size, cy + size, true}, {cx + size, cy + size, true} // 코너
    };

    for (auto& cp : candidates) {
        int tx = cp.x, ty = cp.y;
        if (cp.isCorner) {
            // 코너 문 모델의 왼쪽 면(진입로) 계산
            if (cp.x < cx && cp.y < cy)      tx -= 1; // NW -> West
            else if (cp.x > cx && cp.y < cy) ty -= 1; // NE -> North
            else if (cp.x < cx && cp.y > cy) ty += 1; // SW -> South
            else if (cp.x > cx && cp.y > cy) tx += 1; // SE -> East
        }
        else {
            tx += (cp.x > cx ? 1 : (cp.x < cx ? -1 : 0));
            ty += (cp.y > cy ? 1 : (cp.y < cy ? -1 : 0));
        }

        if (!IsValid(tx, ty)) continue;
        EModelType floor = GetTile(ELayer::FLOOR, tx, ty);
        if (floor != EModelType::WALL && floor != EModelType::HOUSE_INNTER) {
            mapGrid[(int)ELayer::STRUCTURE][cp.y][cp.x] = cp.isCorner ? EModelType::CORNER_DOOR : EModelType::DOOR;
            return true;
        }
    }
    return false;
}

void MapGenerator::PlaceMediumStore(int cx, int cy) {
    int size = HOUSE_SIZE - 1;
    if (!IsValid(cx - (size + 1), cy - (size + 1)) || !IsValid(cx + size + 1, cy + size + 1)) return;
    SetBuildingArea(cx, cy, size, EModelType::STORE);
    TryPlaceDoor(cx, cy, size);
}

void MapGenerator::PlaceLargeWarehouse(int cx, int cy) {
    if (!IsValid(cx - (HOUSE_SIZE + 1), cy - (HOUSE_SIZE + 1)) || !IsValid(cx + HOUSE_SIZE + 1, cy + HOUSE_SIZE + 1)) return;
    SetBuildingArea(cx, cy, HOUSE_SIZE, EModelType::WAREHOUSE);
    TryPlaceDoor(cx, cy, HOUSE_SIZE);
}

bool MapGenerator::IsSpaceForTree(int x, int y) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (GetTile(ELayer::OBJECT, x + dx, y + dy) != EModelType::UNKNOWN) return false;
    return true;
}

void MapGenerator::PlaceParkPlaza(int cx, int cy) {
    if (!IsValid(cx - 3, cy - 3) || !IsValid(cx + 3, cy + 3)) return;

    for (int y = cy - 3; y <= cy + 3; y++)
        for (int x = cx - 3; x <= cx + 3; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::PARK_GREEN;

    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::BENCH;
    if (IsSpaceForTree(cx, cy + 2)) mapGrid[(int)ELayer::OBJECT][cy + 2][cx] = EModelType::SEESAW;

    for (int y = cy - 3; y <= cy + 3; y++) {
        for (int x = cx - 3; x <= cx + 3; x++) {
            if (GetTile(ELayer::OBJECT, x, y) != EModelType::UNKNOWN) continue;
            bool isEdge = (y == cy - 3 || y == cy + 3 || x == cx - 3 || x == cx + 3);
            if (isEdge && rand() % 100 < 50) {
                mapGrid[(int)ELayer::OBJECT][y][x] = IsSpaceForTree(x, y) ? EModelType::TREE : EModelType::SMALL_BUSH;
            }
            else if (!isEdge && rand() % 100 < 10) {
                mapGrid[(int)ELayer::OBJECT][y][x] = EModelType::SMALL_BUSH;
            }
        }
    }
}
// --- 소품 및 건물 배치 함수 ---

void MapGenerator::PlaceSmallKiosk(int cx, int cy) {
    if (!IsValid(cx - 1, cy - 1) || !IsValid(cx + 1, cy + 1)) return;

    // 건물 위거나 문 바로 앞이면 설치 안 함 (여유 공간 확보)
    if (IsBuilding(cx, cy)) return;

    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (mapGrid[(int)ELayer::STRUCTURE][y][x] == EModelType::DOOR ||
                mapGrid[(int)ELayer::STRUCTURE][y][x] == EModelType::CORNER_DOOR) return;
        }
    }

    // 바닥을 마을 길로 교체하고 키오스크 배치
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (!IsBuilding(x, y)) mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
        }
    }
    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::KIOSK;
}

void MapGenerator::PlaceTreasure() {
    const int BLOCK_SIZE = 10;
    // 맵을 구역(Block)으로 나눠서 보물이 한곳에 쏠리지 않게 배치
    for (int by = 0; by < HEIGHT; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            int count = rand() % 3; // 구역당 최대 2개
            for (int t = 0; t < count; t++) {
                int rx = bx + rand() % BLOCK_SIZE;
                int ry = by + rand() % BLOCK_SIZE;

                if (!IsValid(rx, ry)) continue;

                EModelType floor = mapGrid[(int)ELayer::FLOOR][ry][rx];
                if (GetTile(ELayer::OBJECT, rx, ry) == EModelType::UNKNOWN && floor != EModelType::WALL) {
                    mapGrid[(int)ELayer::OBJECT][ry][rx] = EModelType::TREASURE;
                }
            }
        }
    }
}

// --- 유틸리티 및 지형 생성 알고리즘 ---
bool MapGenerator::IsBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    // 모든 건물 관련 타입을 체크
    return (t == EModelType::WAREHOUSE || t == EModelType::STORE ||
        t == EModelType::DOOR || t == EModelType::CORNER_DOOR ||
        t == EModelType::HOUSE_WALL_CORNER || t == EModelType::HOUSE_WALL_STRAIGHT ||
        t == EModelType::HOUSE_WALL_EMPTY);
}

void MapGenerator::RefineBuildingTiles() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType& current = mapGrid[(int)ELayer::STRUCTURE][y][x];
            if (current != EModelType::WAREHOUSE && current != EModelType::STORE) continue;

            int mask = 0;
            if (IsBuilding(x, y - 1)) mask |= 1; // 상
            if (IsBuilding(x, y + 1)) mask |= 2; // 하
            if (IsBuilding(x - 1, y)) mask |= 4; // 좌
            if (IsBuilding(x + 1, y)) mask |= 8; // 우

            if (mask == 15) current = EModelType::HOUSE_WALL_EMPTY; // 사방이 건물임 (내부 벽)
            else if (mask == 10 || mask == 6 || mask == 5 || mask == 9) current = EModelType::HOUSE_WALL_CORNER;
            else current = EModelType::HOUSE_WALL_STRAIGHT;
        }
    }
}

void MapGenerator::CarveMaze(int startX, int startY) {
    std::stack<Cell> s;
    s.push({ startX, startY });
    mapGrid[(int)ELayer::FLOOR][startY][startX] = EModelType::ROAD;

    while (!s.empty()) {
        Cell curr = s.top();
        int dirs[] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++) std::swap(dirs[i], dirs[rand() % 4]);

        bool moved = false;
        for (int i = 0; i < 4; i++) {
            int nx = curr.x + dx[dirs[i]];
            int ny = curr.y + dy[dirs[i]];

            if (IsValid(nx, ny) && mapGrid[(int)ELayer::FLOOR][ny][nx] == EModelType::WALL) {
                // 중간 타일과 대상 타일을 모두 길로 뚫음
                mapGrid[(int)ELayer::FLOOR][curr.y + dy[dirs[i]] / 2][curr.x + dx[dirs[i]] / 2] = EModelType::ROAD;
                mapGrid[(int)ELayer::FLOOR][ny][nx] = EModelType::ROAD;
                s.push({ nx, ny });
                moved = true;
                break;
            }
        }
        if (!moved) s.pop();
    }
}

void MapGenerator::CreateOpenSpaces(int numSpaces) {
    std::vector<Rect> builtSpaces;
    for (int i = 0; i < numSpaces; ) {
        int w = 3 + rand() % 5, h = 3 + rand() % 5;
        int startX = 2 + rand() % (WIDTH - w - 2), startY = 2 + rand() % (HEIGHT - h - 2);
        Rect newSpace = { startX, startY, w, h };

        bool overlap = false;
        for (const auto& existing : builtSpaces) {
            if (newSpace.Intersects(existing)) { overlap = true; break; }
        }

        if (!overlap) {
            for (int y = startY; y < startY + h; y++)
                for (int x = startX; x < startX + w; x++)
                    mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::ROAD;

            builtSpaces.push_back(newSpace);
            i++;
        }
    }
}

bool MapGenerator::IsValid(int x, int y) {
    return (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1);
}