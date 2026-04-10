#ifdef CLIENT
    #include "stdafx.h"
#else
    #include "pch.h"
#endif

#include "MapGenerator.h"

//==================================
// **** 클라/서버 공동 참조 파일 ****
//==================================

using namespace MapGenerator;

// 범위를 포함하는 난수 생성 헬퍼 함수
int GetRandomInt(int min, int max) {
    std::uniform_int_distribution<int> dis(min, max);
    return dis(gen);
}

// Store centers tracked during PlaceMediumStore, used by PlaceMonster
std::vector<Cell> g_store_centers;

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
    g_store_centers.clear();
    float areaRatio = (WIDTH * HEIGHT) / 5000.0f;
    int halfHeight = HEIGHT / 2;

    // 초기화 (벽으로 채우기)
    for (int l = 0; l < (int)ELayer::COUNT; l++)
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                mapGrid[l][y][x] = (l == (int)ELayer::FLOOR) ? EModelType::WALL : EModelType::UNKNOWN;

    // 지형 생성 (미로 + 오픈 스페이스)
    CarveMaze(1, 1);
    CreateOpenSpaces(max(5, (int)(20 * areaRatio)));

    PlaceStructures(areaRatio, halfHeight);

    ApplyAreaTheme(halfHeight);
    PlaceTreasure();
    RefineBuildingTiles();
    PlaceMonster();

    // 인스턴스 데이터 변환
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
                    inst.rotationY = CalculateRotation(x, y, type);
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

void MapGenerator::PlaceStructures(float areaRatio, int halfHeight) {
    for (int i = 0; i < max(1, (int)(3 * areaRatio)); i++)
        PlaceLargeWarehouse(GetRandomInt(4, WIDTH - 4), GetRandomInt(halfHeight, HEIGHT - 4));
    for (int i = 0; i < max(2, (int)(10 * areaRatio)); i++)
        PlaceMediumStore(GetRandomInt(4, WIDTH - 4), GetRandomInt(halfHeight, HEIGHT - 4));
    for (int i = 0; i < max(5, (int)(20 * areaRatio)); i++)
        PlaceParkPlaza(GetRandomInt(3, WIDTH - 3), GetRandomInt(5, halfHeight - 5));
    for (int i = 0; i < max(5, (int)(25 * areaRatio)); i++)
        PlaceSmallKiosk(GetRandomInt(4, WIDTH - 4), GetRandomInt(halfHeight, HEIGHT - 4));
}

void MapGenerator::ApplyAreaTheme(int halfHeight) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType& floor = mapGrid[(int)ELayer::FLOOR][y][x];
            EModelType& structLayer = mapGrid[(int)ELayer::STRUCTURE][y][x];

            if (y > halfHeight) { // 마을 구역
                if (floor == EModelType::ROAD) floor = EModelType::VILLAGE_ROAD;
                else if (floor == EModelType::WALL) {
                    floor = EModelType::VILLAGE_ROAD;
                    structLayer = EModelType::VILLAGE_WALL;
                }
            }
            else { // 공원 구역
                if (floor == EModelType::WALL) {
                    floor = EModelType::ROAD;
                    structLayer = EModelType::PARK_WALL;
                }
            }
        }
    }
}

// 구조물 레이어의 회전값을 계산하는 전용 함수
float MapGenerator::CalculateRotation(int x, int y, EModelType type) {
    int bMask = GetBuildingMask(x, y, true);
    int sMask = GetBuildingMask(x, y, false);

    // 코너형 (주택 코너, 상점 코너, 코너 문)
    if (type == EModelType::HOUSE_WALL_CORNER || type == EModelType::STORE_WALL_CORNER || type == EModelType::CORNER_DOOR) {
        int mask = (type == EModelType::STORE_WALL_CORNER) ? sMask : bMask;
        if ((mask & 10) == 10) return 0.0f;
        if ((mask & 6) == 6)   return 270.0f;
        if ((mask & 5) == 5)   return 180.0f;
        if ((mask & 9) == 9)   return 90.0f;
    }
    // 직선형 (벽, 문)
    else if (type == EModelType::HOUSE_WALL_STRAIGHT || type == EModelType::DOOR) {
        if (!(bMask & 1)) return 0.0f;
        if (!(bMask & 8)) return 270.0f;
        if (!(bMask & 2)) return 180.0f;
        if (!(bMask & 4)) return 90.0f;
    }
    return 0.0f;
}

// --- 로직 함수들 ---
bool MapGenerator::TryPlaceDoor(int cx, int cy, int size) {
    struct DoorPos { int x, y; bool isCorner; };
    std::vector<DoorPos> candidates = {
         {cx, cy - size, false}, {cx, cy + size, false}, {cx - size, cy, false}, {cx + size, cy, false}, // 중앙
        {cx - size, cy - size, true}, {cx + size, cy - size, true}, {cx - size, cy + size, true}, {cx + size, cy + size, true} // 코너
    };

    std::shuffle(candidates.begin(), candidates.end(), gen);

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

    for (int y = cy - size; y <= cy + size; y++) {
        for (int x = cx - size; x <= cx + size; x++) {
            if (mapGrid[(int)ELayer::STRUCTURE][y][x] != EModelType::UNKNOWN) return;
        }
    }

    for (int y = cy - size; y <= cy + size; y++) {
        for (int x = cx - size; x <= cx + size; x++) {
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
            mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::STORE;
        }
    }

    // 상점 탈출구 예약
    if (IsValid(cx + size + 1, cy)) {
        mapGrid[(int)ELayer::FLOOR][cy][cx + size + 1] = EModelType::ROAD;
    }

    g_store_centers.push_back({ cx, cy });
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
    int size = 3;
    if (!IsValid(cx - size, cy - size) || !IsValid(cx + size, cy + size)) return;

    for (int y = cy - size; y <= cy + size; y++)
        for (int x = cx - size; x <= cx + size; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::PARK_GREEN;

    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::BENCH;
    if (IsSpaceForTree(cx, cy + size - 1)) mapGrid[(int)ELayer::OBJECT][cy + size - 1][cx] = EModelType::SEESAW;

    for (int y = cy - size; y <= cy + size; y++) {
        for (int x = cx - size; x <= cx + size; x++) {
            if (GetTile(ELayer::OBJECT, x, y) != EModelType::UNKNOWN) continue;
            bool isEdge = (y == cy - size || y == cy + size || x == cx - size || x == cx + size);
            if (isEdge && GetRandomInt(0, 99) < 50) {
                mapGrid[(int)ELayer::OBJECT][y][x] = IsSpaceForTree(x, y) ? EModelType::TREE : EModelType::SMALL_BUSH;
            }
            else if (!isEdge && GetRandomInt(0, 99) < 10) {
                mapGrid[(int)ELayer::OBJECT][y][x] = EModelType::SMALL_BUSH;
            }
        }
    }
}

// --- 소품 및 건물 배치 함수 ---
void MapGenerator::PlaceSmallKiosk(int cx, int cy) {
    if (!IsValid(cx - 1, cy - 1) || !IsValid(cx + 1, cy + 1)) return;

    // 건물 위거나 문 바로 앞이면 설치 안 함 (여유 공간 확보)
    if (IsHouseBuilding(cx, cy)) return;

    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (mapGrid[(int)ELayer::STRUCTURE][y][x] == EModelType::DOOR ||
                mapGrid[(int)ELayer::STRUCTURE][y][x] == EModelType::CORNER_DOOR) return;
        }
    }

    // 바닥을 마을 길로 교체하고 키오스크 배치
    for (int y = cy - 1; y <= cy + 1; y++) {
        for (int x = cx - 1; x <= cx + 1; x++) {
            if (!IsHouseBuilding(x, y)) mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
        }
    }
    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::KIOSK;
}

void MapGenerator::PlaceMonster()
{
    const int size = HOUSE_SIZE - 1;

    const int ndx[] = { 0, 0, -1, 1 };
    const int ndy[] = { -1, 1,  0, 0 };

    // Ghost: 미로 길(ROAD) 타일에 구역별로 1마리씩 배치
    const int BLOCK_SIZE = 15;
    for (int by = 0; by < HEIGHT / 2; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            for (int attempt = 0; attempt < 20; attempt++) {
                int rx = GetRandomInt(bx, min(bx + BLOCK_SIZE - 1, WIDTH - 1));
                int ry = GetRandomInt(by, min(by + BLOCK_SIZE - 1, HEIGHT / 2 - 1));
                if (!IsValid(rx, ry)) continue;
                if (GetTile(ELayer::FLOOR, rx, ry) != EModelType::ROAD) continue;
                if (GetTile(ELayer::OBJECT, rx, ry) != EModelType::UNKNOWN) continue;
                mapGrid[(int)ELayer::OBJECT][ry][rx] = EModelType::MONSTER_GHOST;
                break;
            }
        }
    }

    for (const Cell& center : g_store_centers) {
        int cx = center.x, cy = center.y;
        bool placed = false;

        // 상점 문 앞(바깥쪽)에 배치 시도
        for (int dy = -(size + 1); dy <= (size + 1) && !placed; dy++) {
            for (int dx = -(size + 1); dx <= (size + 1) && !placed; dx++) {
                int sx = cx + dx, sy = cy + dy;
                if (!IsValid(sx, sy)) continue;

                EModelType str = mapGrid[(int)ELayer::STRUCTURE][sy][sx];
                if (str != EModelType::DOOR && str != EModelType::CORNER_DOOR) continue;

                for (int i = 0; i < 4 && !placed; i++) {
                    int nx = sx + ndx[i], ny = sy + ndy[i];
                    if (!IsValid(nx, ny)) continue;
                    if (IsHouseBuilding(nx, ny)) continue;

                    EModelType floor = GetTile(ELayer::FLOOR, nx, ny);
                    if (floor == EModelType::WALL || floor == EModelType::HOUSE_INNTER) continue;
                    if (GetTile(ELayer::OBJECT, nx, ny) != EModelType::UNKNOWN) continue;

                    mapGrid[(int)ELayer::OBJECT][ny][nx] = EModelType::MONSTER_HUMAN;
                    placed = true;
                }
            }
        }

        // 상점 내부에 배치 (중심 근처의 VILLAGE_ROAD 타일)
        if (!placed) {
            for (int dy = -size; dy <= size && !placed; dy++) {
                for (int dx = -size; dx <= size && !placed; dx++) {
                    int sx = cx + dx, sy = cy + dy;
                    if (!IsValid(sx, sy)) continue;
                    if (GetTile(ELayer::FLOOR, sx, sy) != EModelType::VILLAGE_ROAD) continue;
                    if (GetTile(ELayer::OBJECT, sx, sy) != EModelType::UNKNOWN) continue;

                    mapGrid[(int)ELayer::OBJECT][sy][sx] = EModelType::MONSTER_HUMAN;
                    placed = true;
                }
            }
        }
    }
}

void MapGenerator::PlaceTreasure() {
    const int BLOCK_SIZE = 10;
    // 맵을 구역(Block)으로 나눠서 보물이 한곳에 쏠리지 않게 배치
    for (int by = 0; by < HEIGHT; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            int count = GetRandomInt(0, 2); // 구역당 최대 2개
            for (int t = 0; t < count; t++) {
                int rx = GetRandomInt(bx, min(bx + BLOCK_SIZE - 1, WIDTH - 1));
                int ry = GetRandomInt(by, min(by + BLOCK_SIZE - 1, HEIGHT - 1));

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
bool MapGenerator::IsHouseBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    // 모든 건물 관련 타입을 체크
    return (t == EModelType::WAREHOUSE || t == EModelType::STORE ||
        t == EModelType::DOOR || t == EModelType::CORNER_DOOR ||
        t == EModelType::HOUSE_WALL_CORNER || t == EModelType::HOUSE_WALL_STRAIGHT ||
        t == EModelType::HOUSE_WALL_EMPTY);
}

bool MapGenerator::IsStoreBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    // 모든 건물 관련 타입을 체크
    return (t == EModelType::STORE || t == EModelType::STORE_WALL_CORNER || t == EModelType::STORE_WALL_EMPTY);
}

void MapGenerator::RefineBuildingTiles() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType& current = mapGrid[(int)ELayer::STRUCTURE][y][x];

            // 창고(Warehouse) 처리
            if (bool isHouse = current == EModelType::WAREHOUSE) {
                int mask = GetBuildingMask(x, y, isHouse);

                if (mask == 15) current = EModelType::HOUSE_WALL_EMPTY;
                else if (mask == 10 || mask == 6 || mask == 5 || mask == 9) current = EModelType::HOUSE_WALL_CORNER;
                else current = EModelType::HOUSE_WALL_STRAIGHT;
            }
            // 천막 상점(Store) 처리
            else if (current == EModelType::STORE) {
                int mask = GetBuildingMask(x, y, false);
                if (mask == 10 || mask == 6 || mask == 5 || mask == 9) {
                    current = EModelType::STORE_WALL_CORNER;
                }
                else {
                    current = EModelType::STORE_WALL_EMPTY;
                }
            }
        }
    }
}

// 헬퍼 함수: 주변 4방향 건물 여부 체크
int MapGenerator::GetBuildingMask(int x, int y, bool isHouse) {
    int mask = 0;
    if (isHouse) {
        if (IsHouseBuilding(x, y - 1)) mask |= 1; // 상
        if (IsHouseBuilding(x, y + 1)) mask |= 2; // 하
        if (IsHouseBuilding(x - 1, y)) mask |= 4; // 좌
        if (IsHouseBuilding(x + 1, y)) mask |= 8; // 우
    }
    else {
        if (IsStoreBuilding(x, y - 1)) mask |= 1; // 상
        if (IsStoreBuilding(x, y + 1)) mask |= 2; // 하
        if (IsStoreBuilding(x - 1, y)) mask |= 4; // 좌
        if (IsStoreBuilding(x + 1, y)) mask |= 8; // 우
    }
    return mask;
}

void MapGenerator::CarveMaze(int startX, int startY) {
    std::stack<Cell> s;
    s.push({ startX, startY });

    // 시작점이 이미 ROAD(문 앞)라면 그대로 두고, 아니면 ROAD로 바꿈
    if (mapGrid[(int)ELayer::FLOOR][startY][startX] == EModelType::WALL)
        mapGrid[(int)ELayer::FLOOR][startY][startX] = EModelType::ROAD;

    while (!s.empty()) {
        Cell curr = s.top();
        int dirs[] = { 0, 1, 2, 3 };
        std::shuffle(std::begin(dirs), std::end(dirs), gen);

        bool moved = false;
        for (int i = 0; i < 4; i++) {
            int nx = curr.x + dx[dirs[i]];
            int ny = curr.y + dy[dirs[i]];

            if (IsValid(nx, ny) && mapGrid[(int)ELayer::FLOOR][ny][nx] == EModelType::WALL) {
                // 중간 타일도 체크 (건물을 관통하지 않도록)
                int mx = curr.x + dx[dirs[i]] / 2;
                int my = curr.y + dy[dirs[i]] / 2;

                if (mapGrid[(int)ELayer::STRUCTURE][my][mx] == EModelType::UNKNOWN) {
                    mapGrid[(int)ELayer::FLOOR][my][mx] = EModelType::ROAD;
                    mapGrid[(int)ELayer::FLOOR][ny][nx] = EModelType::ROAD;
                    s.push({ nx, ny });
                    moved = true;
                    break;
                }
            }
        }
        if (!moved) s.pop();
    }
}

void MapGenerator::CreateOpenSpaces(int numSpaces) {
    std::vector<Rect> builtSpaces;
    for (int i = 0; i < numSpaces; ) {
        int w = GetRandomInt(3, 7);
        int h = GetRandomInt(3, 7);
        int startX = GetRandomInt(2, WIDTH - w - 2);
        int startY = GetRandomInt(2, HEIGHT - h - 2);
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
