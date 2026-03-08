#include "stdafx.h"
#include "MapGenerator.h"

using namespace MapGenerator;

// 3D 그리드 초기화 및 맵 생성 메인
std::vector<InstanceData> MapGenerator::Generate3DMap()
{
    srand((unsigned int)time(NULL));
    float areaRatio = (WIDTH * HEIGHT) / 5000.0f;
    int halfHeight = HEIGHT / 2;

    // 1. 모든 레이어 초기화
    for (int l = 0; l < (int)ELayer::COUNT; l++)
        for (int y = 0; y < HEIGHT; y++)
            for (int x = 0; x < WIDTH; x++)
                mapGrid[l][y][x] = EModelType::UNKNOWN;

    // 바닥 레이어 기본값(벽) 설정
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::WALL;

    // 2. 미로 생성 및 길 뚫기 (FLOOR 레이어 작업)
    CarveMaze(1, 1);
    for (int i = 0; i < (WIDTH * HEIGHT) / 3; i++) {
        mapGrid[(int)ELayer::FLOOR][1 + rand() % (HEIGHT - 2)][1 + rand() % (WIDTH - 2)] = EModelType::ROAD;
    }
    CreateOpenSpaces(max(5, (int)(20 * areaRatio)));

    // 3. 구역 테마 설정 (FLOOR 레이어)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mapGrid[(int)ELayer::FLOOR][y][x] == EModelType::ROAD && y > halfHeight) {
                mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
            }
        }
    }

    // 4. 구조물 스폰 (각 레이어에 분산 기록)
    int numWarehouse = max(1, (int)(3 * areaRatio));
    for (int i = 0; i < numWarehouse; i++) PlaceLargeWarehouse(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 10));

    int numStore = max(2, (int)(10 * areaRatio));
    for (int i = 0; i < numStore; i++) PlaceMediumStore(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));

    int numKiosk = max(5, (int)(25 * areaRatio));
    for (int i = 0; i < numKiosk; i++) PlaceSmallKiosk(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));

    int numPark = max(5, (int)(20 * areaRatio));
    for (int i = 0; i < numPark; i++) PlaceParkPlaza(3 + rand() % (WIDTH - 6), 5 + rand() % (halfHeight - 10));

    // 5. 보물 배치 (OBJECT 레이어 활용 - 바닥 위에 겹치기 가능)
    PlaceTreasure();

    // 6. 건물 벽 정밀화 (STRUCTURE 레이어 분석)
    RefineBuildingTiles();

    // 7. 인스턴스 데이터 생성
    const float TILE_SIZE = 2.0f;
    std::vector<InstanceData> instanceList;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            for (int l = 0; l < (int)ELayer::COUNT; l++) {
                EModelType type = mapGrid[l][y][x];
                if (type == EModelType::UNKNOWN) continue;

                InstanceData inst;
                inst.type = type;
                // 바닥과 소품이 겹칠 때 깜빡임을 방지하기 위해 y축 레이어별 오프셋 미세 조정
                float yOffset = (float)l * 0.01f;
                inst.position = XMFLOAT3((float)x * TILE_SIZE, yOffset, (float)y * TILE_SIZE);
                inst.rotationY = 0.0f;

                // 회전 로직 (STRUCTURE 레이어 대상)
                if (l == (int)ELayer::STRUCTURE) {
                    int bMask = 0;
                    if (IsBuilding(x, y - 1)) bMask |= 1;
                    if (IsBuilding(x, y + 1)) bMask |= 2;
                    if (IsBuilding(x - 1, y)) bMask |= 4;
                    if (IsBuilding(x + 1, y)) bMask |= 8;

                    if (type == EModelType::HOUSE_WALL_STRAIGHT) {
                        if (!(bMask & 1)) inst.rotationY = 0.0f;
                        else if (!(bMask & 8)) inst.rotationY = 270.0f;
                        else if (!(bMask & 2)) inst.rotationY = 180.0f;
                        else if (!(bMask & 4)) inst.rotationY = 90.0f;
                    }
                    else if (type == EModelType::HOUSE_WALL_CORNER) {
                        if ((bMask & 10) == 10) inst.rotationY = 0.0f;
                        else if ((bMask & 6) == 6) inst.rotationY = 270.0f;
                        else if ((bMask & 5) == 5) inst.rotationY = 180.0f;
                        else if ((bMask & 9) == 9) inst.rotationY = 90.0f;
                    }
                    else if (type == EModelType::DOOR) {
                        if (bMask & 2) inst.rotationY = 0.0f;
                        else if (bMask & 4) inst.rotationY = 90.0f;
                        else if (bMask & 1) inst.rotationY = 180.0f;
                        else inst.rotationY = 270.0f;
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

// --- 배치 함수들 (레이어 적용) ---

void MapGenerator::PlaceMediumStore(int cx, int cy) {
    if (!IsValid(cx - 2, cy - 2) || !IsValid(cx + 2, cy + 2)) return;
    // 바닥 깔기
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::HOUSE_INNTER;
    // 건물 본체
    for (int y = cy - 1; y <= cy + 1; y++)
        for (int x = cx - 1; x <= cx + 1; x++)
            mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::STORE;
    // 문
    mapGrid[(int)ELayer::STRUCTURE][cy - 1][cx] = EModelType::DOOR;
}

void MapGenerator::PlaceLargeWarehouse(int cx, int cy) {
    if (!IsValid(cx - 3, cy - 3) || !IsValid(cx + 3, cy + 3)) return;
    for (int y = cy - 3; y <= cy + 3; y++)
        for (int x = cx - 3; x <= cx + 3; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::HOUSE_INNTER;
    // 건물 본체
    for (int y = cy - 1; y <= cy + 1; y++)
        for (int x = cx - 1; x <= cx + 1; x++)
            mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::WAREHOUSE;

    mapGrid[(int)ELayer::STRUCTURE][cy - 1][cx] = EModelType::DOOR;
}

void MapGenerator::PlaceParkPlaza(int cx, int cy) {
    if (!IsValid(cx - 2, cy - 2) || !IsValid(cx + 2, cy + 2)) return;

    // 바닥 채우기
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::PARK_GREEN;

    // 벤치/나무
    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::BENCH;
    mapGrid[(int)ELayer::OBJECT][cy][cx - 1] = EModelType::TREE;
    mapGrid[(int)ELayer::OBJECT][cy][cx + 1] = EModelType::TREE;

    // 랜덤
    auto pickRandomObject = []() -> EModelType {
        int r = rand() % 100; // 0~99

        if (r < 5)  return EModelType::SEESAW;
        if (r < 30)  return EModelType::SMALL_BUSH;
        return EModelType::UNKNOWN;
        };

    // 광장 내부에 랜덤 오브젝트 배치
    for (int y = cy - 2; y <= cy + 2; y++) {
        for (int x = cx - 2; x <= cx + 2; x++) {

            // 중앙 벤치 + 나무 자리 제외
            if ((x == cx && y == cy) ||
                (x == cx - 1 && y == cy) ||
                (x == cx + 1 && y == cy))
                continue;

            EModelType obj = pickRandomObject();
            if (obj != EModelType::UNKNOWN)
                mapGrid[(int)ELayer::OBJECT][y][x] = obj;
        }
    }
}

void MapGenerator::PlaceTreasure()
{
    const int BLOCK_SIZE = 10;
    for (int by = 0; by < HEIGHT; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            int treasureCount = rand() % 3;
            for (int t = 0; t < treasureCount; t++) {
                int rx = bx + rand() % BLOCK_SIZE;
                int ry = by + rand() % BLOCK_SIZE;
                if (!IsValid(rx, ry)) continue;

                EModelType floor = mapGrid[(int)ELayer::FLOOR][ry][rx];
                // 길 위나 건물 내부 바닥에 보물 배치
                if (floor == EModelType::ROAD || floor == EModelType::VILLAGE_ROAD || floor == EModelType::HOUSE_INNTER) {
                    if (mapGrid[(int)ELayer::STRUCTURE][ry][rx] == EModelType::UNKNOWN) {
                        mapGrid[(int)ELayer::OBJECT][ry][rx] = EModelType::TREASURE;
                    }
                }
            }
        }
    }
}

void MapGenerator::PlaceSmallKiosk(int cx, int cy) {
    if (!IsValid(cx - 1, cy - 1) || !IsValid(cx + 1, cy + 1)) return;
    for (int y = cy - 1; y <= cy + 1; y++)
        for (int x = cx - 1; x <= cx + 1; x++)
            mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::VILLAGE_ROAD;
    mapGrid[(int)ELayer::OBJECT][cy][cx] = EModelType::KIOSK;
}

// --- 유틸리티 및 정밀화 ---
bool MapGenerator::IsBuilding(int x, int y) {
    if (!IsValid(x, y)) return false;
    EModelType t = mapGrid[(int)ELayer::STRUCTURE][y][x];
    return (t == EModelType::WAREHOUSE || t == EModelType::STORE || t == EModelType::DOOR ||
        t == EModelType::HOUSE_WALL_CORNER || t == EModelType::HOUSE_WALL_STRAIGHT || t == EModelType::HOUSE_WALL_EMPTY);
}

void MapGenerator::RefineBuildingTiles() {
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType current = mapGrid[(int)ELayer::STRUCTURE][y][x];
            if (current != EModelType::WAREHOUSE && current != EModelType::STORE) continue;

            int mask = 0;
            if (IsBuilding(x, y - 1)) mask |= 1;
            if (IsBuilding(x, y + 1)) mask |= 2;
            if (IsBuilding(x - 1, y)) mask |= 4;
            if (IsBuilding(x + 1, y)) mask |= 8;

            if (mask >= 15) mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::HOUSE_WALL_EMPTY;
            else if (mask == 10 || mask == 6 || mask == 5 || mask == 9) mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::HOUSE_WALL_CORNER;
            else mapGrid[(int)ELayer::STRUCTURE][y][x] = EModelType::HOUSE_WALL_STRAIGHT;
        }
    }
}

void MapGenerator::CarveMaze(int startX, int startY) {
    std::stack<Cell> s;
    s.push({ startX, startY });
    mapGrid[(int)ELayer::FLOOR][startY][startX] = EModelType::ROAD;

    while (!s.empty()) {
        Cell current = s.top();
        int dirs[] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++) std::swap(dirs[i], dirs[rand() % 4]);
        bool moved = false;
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[dirs[i]];
            int ny = current.y + dy[dirs[i]];
            if (IsValid(nx, ny) && mapGrid[(int)ELayer::FLOOR][ny][nx] == EModelType::WALL) {
                mapGrid[(int)ELayer::FLOOR][current.y + dy[dirs[i]] / 2][current.x + dx[dirs[i]] / 2] = EModelType::ROAD;
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
        for (const auto& existing : builtSpaces) if (newSpace.Intersects(existing)) { overlap = true; break; }
        if (!overlap) {
            for (int y = startY; y < startY + h; y++) {
                for (int x = startX; x < startX + w; x++) {
                    mapGrid[(int)ELayer::FLOOR][y][x] = EModelType::ROAD;
                }
            }
            builtSpaces.push_back(newSpace);
            i++;
        }
    }
}

bool MapGenerator::IsValid(int x, int y) {
    return (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1);
}