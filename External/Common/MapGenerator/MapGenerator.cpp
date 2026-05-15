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

std::vector<Cell> g_treasure_positions;

std::vector<Cell> m_placedCenters; // 구조물 간 거리 체크


// 특정 레이어의 타일 타입을 안전하게 가져옴
EModelType GetTile(ELayer layer, int x, int y) {
    if (!IsValid(x, y)) return EModelType::UNKNOWN;
    return mapGrid[(int)layer][y][x];
}

// 구조물 간 최소 거리 체크 (공원, 상점 등이 겹치지 x)
bool IsFarEnough(int cx, int cy, int minDistance) {
    for (const auto& center : m_placedCenters) {
        int dist = std::abs(center.x - cx) + std::abs(center.y - cy);
        if (dist < minDistance) return false;
    }
    return true;
}


// --- 개선된 베딩(Padding) 헬퍼 함수 ---
// '벽(WALL)'인 경우에만 길로 바꾸도록 하여, 이미 깔린 공원 바닥이나 다른 길을 덮어쓰지 않습니다.
void ClearPadding(int cx, int cy, int size, EModelType roadType) {
    int padSize = size + 1;
    for (int y = cy - padSize; y <= cy + padSize; y++) {
        for (int x = cx - padSize; x <= cx + padSize; x++) {
            if (!IsValid(x, y)) continue;

            // 이미 배치된 구조물(상점 본체 등)은 건드리지 않습니다.
            EModelType currentStruct = mapGrid[(int)ELayer::STRUCTURE][y][x];
            if (currentStruct != EModelType::UNKNOWN && currentStruct != EModelType::WALL) continue;

            // 핵심 수정: '벽'일 때만 바닥재를 roadType으로 교체하고 벽을 제거합니다.
            // 이렇게 하면 공원 바닥재가 일반 길로 덮이는 것을 막을 수 있습니다.
            if (mapGrid[(int)ELayer::FLOOR][y][x] == EModelType::WALL) {
                mapGrid[(int)ELayer::FLOOR][y][x] = roadType;
                mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::UNKNOWN;
            }
        }
    }
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
    g_treasure_positions.clear();

    float areaRatio = (WIDTH * HEIGHT) / 2500.0f;
    int halfHeight = HEIGHT / 2;

    // 초기화
    for (int l = 0; l < (int)ELayer::COUNT; l++)
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                mapGrid[l][y][x] = (l == (int)ELayer::FLOOR) ? EModelType::WALL : EModelType::UNKNOWN;

    // 1. 미로 생성
    CarveMaze(1, 1);

    // 2. 공터 생성
    //CreateOpenSpaces(max(2, (int)(6 * areaRatio)));

    // 3. 구조물 배치
    PlaceStructures(areaRatio, halfHeight);

    // 4. 후처리
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

                float posX = x * TILE_SIZE;
                float posZ = y * TILE_SIZE;
                float rotY = 0.0f;

                // 회전/ 위치 처리
                if (type == EModelType::FENCE_WOOD_STR) {
                    // 나무 울타리 0/90도 정렬
                    bool hasGreenLeft = (GetTile(ELayer::FLOOR, x - 1, y) == EModelType::PARK_GREEN);
                    bool hasGreenRight = (GetTile(ELayer::FLOOR, x + 1, y) == EModelType::PARK_GREEN);
                    rotY = (hasGreenLeft && hasGreenRight) ? 0.0f : 90.0f;
                }
                else if (type == EModelType::KIOSK) {
                    // 주변 타일을 살펴서 자판기 뒷면이 벽을 향하도록 회전 설정
                    if (GetTile(ELayer::FLOOR, x, y - 1) == EModelType::WALL) rotY = 0.0f;    // 뒤가 위쪽 벽
                    else if (GetTile(ELayer::FLOOR, x, y + 1) == EModelType::WALL) rotY = 180.0f; // 뒤가 아래쪽 벽
                    else if (GetTile(ELayer::FLOOR, x - 1, y) == EModelType::WALL) rotY = 270.0f; // 뒤가 왼쪽 벽
                    else if (GetTile(ELayer::FLOOR, x + 1, y) == EModelType::WALL) rotY = 90.0f;  // 뒤가 오른쪽 벽
                    else {
                        // 벽을 못 찾았다면:: 기존처럼 90도 스냅 랜덤
                        static float snapRots[] = { 0.0f, 90.0f, 180.0f, 270.0f };
                        rotY = snapRots[GetRandomInt(0, 3)];
                    }
                }
                else if (type == EModelType::HOUSE_INNTER) {
                    static float snapRots[] = { 0.0f, 90.0f, 180.0f, 270.0f };
                    rotY = snapRots[GetRandomInt(0, 3)];
                }
                else if (l == (int)ELayer::OBJECT) {
                    rotY = (float)GetRandomInt(0, 359);
                    if (type == EModelType::SMALL_BUSH) { // 덤불만 지터
                        float jitter = TILE_SIZE * 0.3f;
                        posX += (GetRandomInt(-100, 100) / 100.0f) * jitter;
                        posZ += (GetRandomInt(-100, 100) / 100.0f) * jitter;
                    }
                }
                else if (l == (int)ELayer::STRUCTURE) {
                    rotY = CalculateRotation(x, y, type);
                }

                inst.position = XMFLOAT3(posX, l * 0.01f, posZ);
                inst.rotationY = rotY;
                instanceList.push_back(inst);


                if (l == (int)ELayer::FLOOR) {

                    if (type == EModelType::ROAD) {

                        InstanceData sand;
                        sand.type = EModelType::PARK_SAND_DECO;

                        float sandY = 0.012f + (GetRandomInt(0, 60) * 0.0001f);
                        sand.position = XMFLOAT3(x * TILE_SIZE, sandY, y * TILE_SIZE);
                        sand.rotationY = (float)GetRandomInt(0, 359);
                        instanceList.push_back(sand);
                    }

                    if (type == EModelType::PARK_GREEN) {
                        for (int i = 0; i < 3; i++) { // 한 칸에 3개 생성
                            InstanceData grass;
                            grass.type = EModelType::GRASS;

                            float dj = TILE_SIZE * 0.4f;
                            float dx = (GetRandomInt(-100, 100) / 100.0f) * dj;
                            float dz = (GetRandomInt(-100, 100) / 100.0f) * dj;

                            grass.position = XMFLOAT3((x * TILE_SIZE) + dx, 0.02f, (y * TILE_SIZE) + dz);
                            grass.rotationY = (float)GetRandomInt(0, 359);
                            instanceList.push_back(grass);
                        }
                    }
                    // 일반 길 20% 확률로 돌맹이 1개
                    else if ((type == EModelType::ROAD) && GetRandomInt(0, 99) < 20) {
                        InstanceData stone;
                        stone.type = EModelType::DECO_STONE;
                        float dj = TILE_SIZE * 0.3f;
                        float dx = (GetRandomInt(-100, 100) / 100.0f) * dj;
                        float dz = (GetRandomInt(-100, 100) / 100.0f) * dj;
                        stone.position = XMFLOAT3((x * TILE_SIZE) + dx, 0.02f, (y * TILE_SIZE) + dz);
                        stone.rotationY = (float)GetRandomInt(0, 359);
                        instanceList.push_back(stone);
                    }
                }
            }
        }
    }

    std::sort(instanceList.begin(), instanceList.end(), [](const InstanceData& a, const InstanceData& b) {
        return (int)a.type < (int)b.type;
        });

    return instanceList;
}





void MapGenerator::PlaceStructures(float areaRatio, int halfHeight) {
    m_placedCenters.clear();

    // 1. 대형 창고 (마을 구역, 거리 12)
    int warehouseCount = max(1, (int)(2 * areaRatio));
    for (int i = 0; i < warehouseCount; i++) {
        for (int attempt = 0; attempt < 20; attempt++) {
            int rx = GetRandomInt(5, WIDTH - 5), ry = GetRandomInt(halfHeight + 2, HEIGHT - 5);
            if (IsFarEnough(rx, ry, 12)) {
                PlaceLargeWarehouse(rx, ry);
                m_placedCenters.push_back({ rx, ry });
                break;
            }
        }
    }

    // 2. 공원 광장 (공원 구역, 거리 10)
    int parkCount = max(2, (int)(6 * areaRatio));
    for (int i = 0; i < parkCount; i++) {
        for (int attempt = 0; attempt < 20; attempt++) {
            int rx = GetRandomInt(4, WIDTH - 4), ry = GetRandomInt(4, halfHeight - 4);
            if (IsFarEnough(rx, ry, 10)) {
                PlaceParkPlaza(rx, ry);
                m_placedCenters.push_back({ rx, ry });
                break;
            }
        }
    }

    // 3. 중형 상점 (마을 구역, 거리 8)
    int storeCount = max(2, (int)(4 * areaRatio));
    for (int i = 0; i < storeCount; i++) {
        for (int attempt = 0; attempt < 20; attempt++) {
            int rx = GetRandomInt(4, WIDTH - 4), ry = GetRandomInt(halfHeight + 2, HEIGHT - 4);
            if (IsFarEnough(rx, ry, 8)) {
                PlaceMediumStore(rx, ry);
                m_placedCenters.push_back({ rx, ry });
                break;
            }
        }
    }

    // 4. 소형 키오스크 (마을 구역, 거리 5)
    int kioskCount = max(3, (int)(10 * areaRatio));
    for (int i = 0; i < kioskCount; i++) {
        for (int attempt = 0; attempt < 15; attempt++) {
            int rx = GetRandomInt(4, WIDTH - 4), ry = GetRandomInt(halfHeight + 1, HEIGHT - 4);
            if (IsFarEnough(rx, ry, 1)) {
                PlaceSmallKiosk(rx, ry);
                m_placedCenters.push_back({ rx, ry });
                break;
            }
        }
    }
}





void MapGenerator::ApplyAreaTheme(int halfHeight) {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType& floor = mapGrid[(int)ELayer::FLOOR][y][x];
            EModelType& structLayer = mapGrid[(int)ELayer::STRUCTURE][y][x];

            if (structLayer != EModelType::UNKNOWN && structLayer != EModelType::WALL) continue;

            if (y > halfHeight) { // 마을 구역
                if (floor == EModelType::ROAD) {
                    floor = EModelType::VILLAGE_ROAD;
                }
                else if (floor == EModelType::WALL) {
                    structLayer = EModelType::VILLAGE_WALL;
                }
            }
            else { // 공원 구역
                // 공원 구역이지만 Plaza(PARK_GREEN)가 아닌 일반 미로 벽/길 처리
                if (floor == EModelType::WALL) {
                    floor = EModelType::ROAD;
                    structLayer = EModelType::PARK_WALL;
                }
            }
        }
    }
}

float MapGenerator::CalculateRotation(int x, int y, EModelType type) {
    int bMask = GetBuildingMask(x, y, true);
    int sMask = GetBuildingMask(x, y, false);

    if (type == EModelType::HOUSE_WALL_CORNER || type == EModelType::STORE_WALL_CORNER || type == EModelType::CORNER_DOOR) {
        int mask = (type == EModelType::STORE_WALL_CORNER) ? sMask : bMask;
        if ((mask & 10) == 10) return 0.0f;
        if ((mask & 6) == 6)   return 270.0f;
        if ((mask & 5) == 5)   return 180.0f;
        if ((mask & 9) == 9)   return 90.0f;
    }
    else if (type == EModelType::HOUSE_WALL_STRAIGHT || type == EModelType::DOOR) {
        if (!(bMask & 1)) return 0.0f;
        if (!(bMask & 8)) return 270.0f;
        if (!(bMask & 2)) return 180.0f;
        if (!(bMask & 4)) return 90.0f;
    }
    return 0.0f;
}

bool MapGenerator::TryPlaceDoor(int cx, int cy, int size) {
    struct DoorPos { int x, y; bool isCorner; };
    std::vector<DoorPos> candidates = {
         {cx, cy - size, false}, {cx, cy + size, false}, {cx - size, cy, false}, {cx + size, cy, false},
        {cx - size, cy - size, true}, {cx + size, cy - size, true}, {cx - size, cy + size, true}, {cx + size, cy + size, true}
    };

    std::shuffle(candidates.begin(), candidates.end(), gen);

    for (auto& cp : candidates) {
        int tx = cp.x, ty = cp.y;
        if (cp.isCorner) {
            if (cp.x < cx && cp.y < cy)      tx -= 1;
            else if (cp.x > cx && cp.y < cy) ty -= 1;
            else if (cp.x < cx && cp.y > cy) ty += 1;
            else if (cp.x > cx && cp.y > cy) tx += 1;
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
    int size = 1;
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

    g_store_centers.push_back({ cx, cy });

    // 상점 주변 베딩 (마을 길로)
    ClearPadding(cx, cy, size, EModelType::VILLAGE_ROAD);
}

void MapGenerator::PlaceLargeWarehouse(int cx, int cy) {
    if (!IsValid(cx - (HOUSE_SIZE + 1), cy - (HOUSE_SIZE + 1)) || !IsValid(cx + HOUSE_SIZE + 1, cy + HOUSE_SIZE + 1)) return;
    SetBuildingArea(cx, cy, HOUSE_SIZE, EModelType::WAREHOUSE);
    TryPlaceDoor(cx, cy, HOUSE_SIZE);

    // 창고 주변 베딩 (마을 길로)
    ClearPadding(cx, cy, HOUSE_SIZE, EModelType::VILLAGE_ROAD);
}

bool MapGenerator::IsSpaceForTree(int x, int y) {
    for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
            if (GetTile(ELayer::OBJECT, x + dx, y + dy) != EModelType::UNKNOWN) return false;
    return true;
}

void MapGenerator::PlaceParkPlaza(int cx, int cy) {
    int size = 2; // 5x5 사이즈
    if (!IsValid(cx - size, cy - size) || !IsValid(cx + size, cy + size)) return;

    // 1. 공원 바닥 깔기
    for (int y = cy - size; y <= cy + size; y++)
        for (int x = cx - size; x <= cx + size; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::PARK_GREEN;

    // 2. 중앙 벤치
    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::BENCH;

    // 3. 테두리 울타리 및 식생 배치
    for (int y = cy - size; y <= cy + size; y++) {
        for (int x = cx - size; x <= cx + size; x++) {
            if (GetTile(ELayer::OBJECT, x, y) != EModelType::UNKNOWN) continue;

            bool isEdgeX = (x == cx - size || x == cx + size);
            bool isEdgeY = (y == cy - size || y == cy + size);

            if ((isEdgeX || isEdgeY) && !(isEdgeX && isEdgeY)) {
                if (GetRandomInt(0, 99) < 50) {
                    // [수정] 나무 울타리 전용 타입으로 배치
                    mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::FENCE_WOOD_STR;
                }
                else {
                    mapGrid[(int)ELayer::OBJECT][y][x] = EModelType::SMALL_BUSH;
                }
            }
            else if (isEdgeX && isEdgeY) {
                mapGrid[(int)ELayer::OBJECT][y][x] = EModelType::TREE;
            }
        }
    }
    ClearPadding(cx, cy, size, EModelType::PARK_GREEN);
}

void MapGenerator::PlaceSmallKiosk(int cx, int cy) {
    if (!IsValid(cx - 1, cy - 1) || !IsValid(cx + 1, cy + 1)) return;
    if (IsHouseBuilding(cx, cy)) return;

    // 1. 자판기가 등질 벽 탐색
    int wallDX = 0, wallDY = 0;
    bool foundWall = false;

    if (GetTile(ELayer::FLOOR, cx, cy - 1) == EModelType::WALL) { wallDY = -1; foundWall = true; }
    else if (GetTile(ELayer::FLOOR, cx, cy + 1) == EModelType::WALL) { wallDY = 1; foundWall = true; }
    else if (GetTile(ELayer::FLOOR, cx - 1, cy) == EModelType::WALL) { wallDX = -1; foundWall = true; }
    else if (GetTile(ELayer::FLOOR, cx + 1, cy) == EModelType::WALL) { wallDX = 1; foundWall = true; }

    if (!foundWall) return;

    // 2. 2x3 영역을 길(VILLAGE_ROAD)로
    if (wallDY != 0) {
        for (int y = cy; (wallDY == -1 ? y <= cy + 1 : y >= cy - 1); (wallDY == -1 ? y++ : y--)) {
            for (int x = cx - 1; x <= cx + 1; x++) {
                if (IsValid(x, y)) mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
            }
        }
    }
    else { // 좌/우 벽에 붙은 경우 (가로 2, 세로 3)
        for (int x = cx; (wallDX == -1 ? x <= cx + 1 : x >= cx - 1); (wallDX == -1 ? x++ : x--)) {
            for (int y = cy - 1; y <= cy + 1; y++) {
                if (IsValid(x, y)) mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
            }
        }
    }

    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::KIOSK;
}



void MapGenerator::PlaceMonster() {
    const int size = 1;
    const int ndx[] = { 0, 0, -1, 1 };
    const int ndy[] = { -1, 1,  0, 0 };

    const int BLOCK_SIZE = 8;
    for (int by = 0; by < HEIGHT / 2; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            for (int attempt = 0; attempt < 10; attempt++) {
                int rx = GetRandomInt(bx, min(bx + BLOCK_SIZE - 1, WIDTH - 1));
                int ry = GetRandomInt(by, min(by + BLOCK_SIZE - 1, HEIGHT / 2 - 1));
                if (!IsValid(rx, ry)) continue;
                if (GetTile(ELayer::FLOOR, rx, ry) != EModelType::ROAD) continue;
                if (GetTile(ELayer::OBJECT, rx, ry) != EModelType::UNKNOWN) continue;
                if (IsBlockedStructure(rx, ry)) continue;

                mapGrid[(int)ELayer::OBJECT][ry][rx] = EModelType::MONSTER_GHOST;
                break;
            }
        }
    }

    for (const Cell& center : g_store_centers) {
        int cx = center.x, cy = center.y;
        bool placed = false;

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
                    if (IsBlockedStructure(nx, ny)) continue;

                    mapGrid[(int)ELayer::OBJECT][ny][nx] = EModelType::MONSTER_HUMAN;
                    placed = true;
                }
            }
        }

        if (!placed) {
            for (int dy = -size; dy <= size && !placed; dy++) {
                for (int dx = -size; dx <= size && !placed; dx++) {
                    int sx = cx + dx, sy = cy + dy;
                    if (!IsValid(sx, sy)) continue;
                    if (GetTile(ELayer::FLOOR, sx, sy) != EModelType::VILLAGE_ROAD) continue;
                    if (GetTile(ELayer::OBJECT, sx, sy) != EModelType::UNKNOWN) continue;
                    if (IsBlockedStructure(sx, sy)) continue;

                    mapGrid[(int)ELayer::OBJECT][sy][sx] = EModelType::MONSTER_HUMAN;
                    placed = true;
                }
            }
        }
    }

    // DogMonster: 보물 인접 타일에 40% 확률로 배치
    for (const Cell& tc : g_treasure_positions) {
        if (GetRandomInt(0, 99) >= 40) continue;

        for (int i = 0; i < 4; i++) {
            int nx = tc.x + ndx[i], ny = tc.y + ndy[i];
            if (!IsValid(nx, ny)) continue;
            if (!IsWalkableFloor(nx, ny)) continue;
            if (IsBlockedStructure(nx, ny)) continue;
            if (GetTile(ELayer::OBJECT, nx, ny) != EModelType::UNKNOWN) continue;

            mapGrid[(int)ELayer::OBJECT][ny][nx] = EModelType::MONSTER_DOG;
            break;
        }
    }
}

void MapGenerator::PlaceTreasure() {
    std::vector<Cell> candidates;
    for (int y = 1; y < HEIGHT - 1; y++) {
        for (int x = 1; x < WIDTH - 1; x++) {
            if (IsWalkableFloor(x, y) && !IsBlockedStructure(x, y) &&
                GetTile(ELayer::OBJECT, x, y) == EModelType::UNKNOWN) {
                candidates.push_back({ x, y });
            }
        }
    }

    std::shuffle(candidates.begin(), candidates.end(), gen);

    const int MIN_DIST = 3;
    const int target   = GetRandomInt(30, 40);

    for (const auto& c : candidates) {
        if ((int)g_treasure_positions.size() >= target) break;

        bool tooClose = false;
        for (const auto& p : g_treasure_positions) {
            if (std::abs(c.x - p.x) + std::abs(c.y - p.y) < MIN_DIST) {
                tooClose = true;
                break;
            }
        }
        if (tooClose) continue;

        mapGrid[(int)ELayer::OBJECT][c.y][c.x] = EModelType::TREASURE;
        g_treasure_positions.push_back(c);
    }
}

bool MapGenerator::IsHouseBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    return (t == EModelType::WAREHOUSE || t == EModelType::STORE ||
        t == EModelType::DOOR || t == EModelType::CORNER_DOOR ||
        t == EModelType::HOUSE_WALL_CORNER || t == EModelType::HOUSE_WALL_STRAIGHT ||
        t == EModelType::HOUSE_WALL_EMPTY);
}

bool MapGenerator::IsBlockedStructure(int x, int y) {
    if (!IsValid(x, y)) return true;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    return (t == EModelType::VILLAGE_WALL ||
        t == EModelType::PARK_WALL ||
        t == EModelType::WAREHOUSE ||
        t == EModelType::STORE ||
        t == EModelType::HOUSE_WALL_STRAIGHT ||
        t == EModelType::HOUSE_WALL_CORNER ||
        t == EModelType::STORE_WALL_CORNER ||
        t == EModelType::FENCE_WOOD_STR);
}

bool MapGenerator::IsStoreBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    return (t == EModelType::STORE || t == EModelType::STORE_WALL_CORNER || t == EModelType::STORE_WALL_EMPTY);
}

void MapGenerator::RefineBuildingTiles() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType& current = mapGrid[(int)ELayer::STRUCTURE][y][x];

            if (bool isHouse = current == EModelType::WAREHOUSE) {
                int mask = GetBuildingMask(x, y, isHouse);
                if (mask == 15) current = EModelType::HOUSE_WALL_EMPTY;
                else if (mask == 10 || mask == 6 || mask == 5 || mask == 9) current = EModelType::HOUSE_WALL_CORNER;
                else current = EModelType::HOUSE_WALL_STRAIGHT;
            }
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

int MapGenerator::GetBuildingMask(int x, int y, bool isHouse) {
    int mask = 0;
    if (isHouse) {
        if (IsHouseBuilding(x, y - 1)) mask |= 1;
        if (IsHouseBuilding(x, y + 1)) mask |= 2;
        if (IsHouseBuilding(x - 1, y)) mask |= 4;
        if (IsHouseBuilding(x + 1, y)) mask |= 8;
    }
    else {
        if (IsStoreBuilding(x, y - 1)) mask |= 1;
        if (IsStoreBuilding(x, y + 1)) mask |= 2;
        if (IsStoreBuilding(x - 1, y)) mask |= 4;
        if (IsStoreBuilding(x + 1, y)) mask |= 8;
    }
    return mask;
}

void MapGenerator::CarveMaze(int startX, int startY) {
    std::stack<Cell> s;
    s.push({ startX, startY });

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
        int w = GetRandomInt(2, 4);
        int h = GetRandomInt(2, 4);
        int startX = GetRandomInt(2, WIDTH - w - 2);
        int startY = GetRandomInt(2, HEIGHT - h - 2);
        Rect newSpace = { startX, startY, w, h };

        bool overlap = false;
        for (const auto& existing : builtSpaces) {
            if (newSpace.Intersects(existing)) { overlap = true; break; }
        }

        if (!overlap) {
            for (int y = startY; y < startY + h; y++) {
                for (int x = startX; x < startX + w; x++) {
                    if (IsValid(x, y))
                        mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::ROAD;
                }
            }

            builtSpaces.push_back(newSpace);
            i++;
        }
        else {
            static int failCount = 0;
            if (++failCount > 100) break;
        }
    }
}

bool MapGenerator::IsValid(int x, int y) {
    return (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1);
}

bool MapGenerator::IsWalkableFloor(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType tile = mapGrid[(int)ELayer::FLOOR][y][x];
    return tile == EModelType::ROAD
        || tile == EModelType::PARK_GREEN
        || tile == EModelType::VILLAGE_ROAD;
}

bool MapGenerator::IsBlockedObject(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType obj = mapGrid[(int)ELayer::OBJECT][y][x];
    return obj == EModelType::TREE
        || obj == EModelType::BENCH
        || obj == EModelType::SEESAW
        || obj == EModelType::KIOSK;
}

std::vector<MapGenerator::Cell> MapGenerator::FindPath(int sx, int sy, int ex, int ey) {
    if (!IsValid(sx, sy) || !IsValid(ex, ey)) return {};

    auto passable = [](int x, int y) {
        return IsWalkableFloor(x, y)
            && !IsBlockedStructure(x, y)
            && !IsBlockedObject(x, y);
        };

    if (!passable(ex, ey)) {
        const int ndx4[] = { 0, 0, -1, 1 };
        const int ndy4[] = { -1, 1,  0, 0 };
        bool found = false;
        for (int i = 0; i < 4 && !found; i++) {
            int tx = ex + ndx4[i];
            int ty = ey + ndy4[i];
            if (IsValid(tx, ty) && passable(tx, ty)) {
                ex = tx; ey = ty;
                found = true;
            }
        }
        if (!found) return {};
    }

    const int ndx[] = { 0, 0, -1, 1 };
    const int ndy[] = { -1, 1,  0, 0 };

    std::vector<std::vector<bool>>  visited(HEIGHT, std::vector<bool>(WIDTH, false));
    std::vector<std::vector<Cell>>  parent(HEIGHT, std::vector<Cell>(WIDTH, { -1, -1 }));

    std::queue<Cell> q;
    q.push({ sx, sy });
    visited[sy][sx] = true;

    while (!q.empty()) {
        Cell cur = q.front(); q.pop();
        if (cur.x == ex && cur.y == ey) {
            std::vector<Cell> path;
            Cell c = { ex, ey };
            while (c.x != sx || c.y != sy) {
                path.push_back(c);
                c = parent[c.y][c.x];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }
        for (int i = 0; i < 4; i++) {
            int nx = cur.x + ndx[i];
            int ny = cur.y + ndy[i];
            if (IsValid(nx, ny) && !visited[ny][nx] && passable(nx, ny)) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({ nx, ny });
            }
        }
    }

    // 목적지에 도달 불가 (예: 펜스로 완전히 막힌 공원) → 탐색된 타일 중 목적지에 가장 가까운 타일로 경로 반환
    Cell best = { sx, sy };
    int  bestDistSq = INT_MAX;
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (!visited[y][x]) continue;
            int dx = x - ex, dy = y - ey;
            int distSq = dx * dx + dy * dy;
            if (distSq < bestDistSq) {
                bestDistSq = distSq;
                best = { x, y };
            }
        }
    }

    if (best.x == sx && best.y == sy) return {};

    std::vector<Cell> path;
    Cell c = best;
    while (c.x != sx || c.y != sy) {
        path.push_back(c);
        c = parent[c.y][c.x];
    }
    std::reverse(path.begin(), path.end());
    return path;
}