#include "stdafx.h"
#include "MapGenerator.h"

std::vector < MapGenerator::InstanceData > MapGenerator::Generate3DMap()
{
    std::vector<InstanceData> instanceList;
    srand((unsigned int)time(NULL));

    float areaRatio = (WIDTH * HEIGHT) / 5000.0f;
    int halfHeight = HEIGHT / 2;

    // 1. 맵 전체를 벽으로 초기화
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) mapGrid[y][x] = EModelType::WALL;

    // 2. 미로 생성 및 랜덤 길 뚫기
    CarveMaze(1, 1);

    for (int i = 0; i < (WIDTH * HEIGHT) / 3; i++) {
        mapGrid[1 + rand() % (HEIGHT - 2)][1 + rand() % (WIDTH - 2)] = EModelType::ROAD;
    }

    CreateOpenSpaces(max(5, (int)(20 * areaRatio)));

    // 3. 구역 테마 설정 (아래쪽은 나무 구역으로 변경)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mapGrid[y][x] == EModelType::WALL && y < halfHeight) {
                mapGrid[y][x] = EModelType::TREE;
            }
        }
    }

    // 4. 구조물 스폰
    int numWarehouse = max(1, (int)(3 * areaRatio));
    for (int i = 0; i < numWarehouse; i++) {
        PlaceLargeWarehouse(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 10));
    }

    int numStore = max(2, (int)(10 * areaRatio));
    for (int i = 0; i < numStore; i++) {
        PlaceMediumStore(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));
    }

    int numKiosk = max(5, (int)(25 * areaRatio));
    for (int i = 0; i < numKiosk; i++) {
        PlaceSmallKiosk(4 + rand() % (WIDTH - 8), halfHeight + rand() % (halfHeight - 5));
    }

    int numPark = max(5, (int)(20 * areaRatio));
    for (int i = 0; i < numPark; i++) {
        PlaceParkPlaza(3 + rand() % (WIDTH - 6), 5 + rand() % (halfHeight - 10));
    }

    // 5. 보물 배치 로직 (비어있는 곳 탐색)
    const int BLOCK_SIZE = 10;
    for (int by = 0; by < HEIGHT; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            int treasureCount = rand() % 4;
            std::vector<std::pair<int, int>> emptySpaces;
            for (int y = by; y < by + BLOCK_SIZE && y < HEIGHT; y++) {
                for (int x = bx; x < bx + BLOCK_SIZE && x < WIDTH; x++) {
                    EModelType type = mapGrid[y][x];
                    // 길이나 가판대/상점 주변에 보물 배치 가능
                    if (type == EModelType::ROAD || type == EModelType::KIOSK || type == EModelType::STORE) {
                        emptySpaces.push_back({ x, y });
                    }
                }
            }
            for (int t = 0; t < treasureCount && !emptySpaces.empty(); t++) {
                int randIdx = rand() % (int)emptySpaces.size();
                mapGrid[emptySpaces[randIdx].second][emptySpaces[randIdx].first] = EModelType::TREASURE;
                emptySpaces.erase(emptySpaces.begin() + randIdx);
            }
        }
    }

    // 6. 3D InstanceData 생성
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            EModelType type = mapGrid[y][x];

            InstanceData inst;
            inst.type = type;
            float posX = (float)x * 1.0f;
            float posZ = (float)y * 1.0f;

            switch (type) {
            case EModelType::ROAD:
                inst.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 0.0f, posZ);
                break;
            case EModelType::WALL:
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
                break;
            case EModelType::WAREHOUSE:
                inst.scale = XMFLOAT3(1.0f, 6.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 3.0f, posZ);
                break;
            case EModelType::STORE:
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
                break;
            case EModelType::KIOSK:
                inst.scale = XMFLOAT3(1.0f, 2.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.0f, posZ);
                break;
            case EModelType::TREE:
                inst.scale = XMFLOAT3(0.8f, 1.5f, 0.8f);
                inst.position = XMFLOAT3(posX, 0.75f, posZ);
                break;
            case EModelType::TREASURE:
                inst.scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
                inst.position = XMFLOAT3(posX, 0.25f, posZ);
                break;
            case EModelType::BENCH:
                inst.scale = XMFLOAT3(0.8f, 0.4f, 0.4f);
                inst.position = XMFLOAT3(posX, 0.2f, posZ);
                break;
            case EModelType::DOOR:
                inst.scale = XMFLOAT3(0.8f, 2.0f, 0.2f);
                inst.position = XMFLOAT3(posX, 1.0f, posZ);
                break;
            default:
                inst.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 0.5f, posZ);
                break;
            }
            instanceList.push_back(inst);
        }
    }

    std::sort(instanceList.begin(), instanceList.end(), [](const InstanceData& a, const InstanceData& b) {
        return (int)a.type < (int)b.type;
        });

    //std::ofstream out("../Modeling/instData.txt");
    //for (int y = 0; y < HEIGHT; y++) {
    //    for (int x = 0; x < WIDTH; x++) {
    //        // NONE은 0, WALL은 1 등으로 출력됨
    //        out << (int)mapGrid[y][x] << " ";
    //    }
    //    out << "\n";
    //}

    return instanceList;
}

// --- 보조 함수들 (기존 로직 유지하되 enum 적용) ---
void MapGenerator::CarveMaze(int startX, int startY)
{
    struct Cell { int x, y; };
    std::stack<Cell> s;
    s.push({ startX, startY });
    mapGrid[startY][startX] = EModelType::ROAD;

    while (!s.empty()) {
        Cell current = s.top();
        int dirs[] = { 0, 1, 2, 3 };
        for (int i = 0; i < 4; i++) std::swap(dirs[i], dirs[rand() % 4]);

        bool moved = false;
        for (int i = 0; i < 4; i++) {
            int nx = current.x + dx[dirs[i]];
            int ny = current.y + dy[dirs[i]];

            if (IsValid(nx, ny) && mapGrid[ny][nx] == EModelType::WALL) {
                mapGrid[current.y + dy[dirs[i]] / 2][current.x + dx[dirs[i]] / 2] = EModelType::ROAD;
                mapGrid[ny][nx] = EModelType::ROAD;
                s.push({ nx, ny });
                moved = true;
                break;
            }
        }
        if (!moved) s.pop();
    }
}

void MapGenerator::PlaceMediumStore(int cx, int cy)
{
    if (!IsValid(cx - 2, cy - 2) || !IsValid(cx + 2, cy + 2)) return;
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = EModelType::ROAD;
    for (int y = cy - 1; y <= cy + 1; y++)
        for (int x = cx - 1; x <= cx + 1; x++) mapGrid[y][x] = EModelType::STORE;

    int doorDir = rand() % 4;
    if (doorDir == 0) mapGrid[cy - 1][cx] = EModelType::DOOR;
    else if (doorDir == 1) mapGrid[cy + 1][cx] = EModelType::DOOR;
    else if (doorDir == 2) mapGrid[cy][cx - 1] = EModelType::DOOR;
    else mapGrid[cy][cx + 1] = EModelType::DOOR;
}

void MapGenerator::PlaceParkPlaza(int cx, int cy)
{
    if (!IsValid(cx - 2, cy - 2) || !IsValid(cx + 2, cy + 2)) return;
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = EModelType::ROAD;
    mapGrid[cy][cx] = EModelType::BENCH;
    mapGrid[cy][cx - 1] = EModelType::TREE;
    mapGrid[cy][cx + 1] = EModelType::TREE;
}

// 좌표가 맵 범위를 벗어나지 않는지 체크
bool MapGenerator::IsValid(int x, int y)
{
    return (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1);
}

// 미로 사이사이에 빈 공간(공터)을 만드는 함수
void MapGenerator::CreateOpenSpaces(int numSpaces)
{
    std::vector<Rect> builtSpaces;
    int attempts = 0;

    for (int i = 0; i < numSpaces && attempts < 1000; ) {
        int w, h;
        int sizeType = rand() % 3;

        // 공터 크기 결정
        if (sizeType == 0) { w = 3 + rand() % 4; h = 3 + rand() % 4; }
        else if (sizeType == 1) { w = 7 + rand() % 5; h = 7 + rand() % 5; }
        else { w = 12 + rand() % 5; h = 12 + rand() % 5; }

        w = min(w, WIDTH - 6);
        h = min(h, HEIGHT - 6);

        int startX = 2 + rand() % (WIDTH - w - 2);
        int startY = 2 + rand() % (HEIGHT - h - 2);

        Rect newSpace = { startX, startY, w, h };
        bool overlap = false;

        for (const auto& existing : builtSpaces) {
            if (newSpace.Intersects(existing)) {
                overlap = true;
                break;
            }
        }

        if (!overlap) {
            // 공터 영역을 ROAD(빈 공간)으로 채움
            for (int y = startY; y < startY + h; y++) {
                for (int x = startX; x < startX + w; x++) {
                    mapGrid[y][x] = EModelType::ROAD;
                }
            }
            builtSpaces.push_back(newSpace);
            i++;
        }
        attempts++;
    }
}

// 소형 가판대 배치
void MapGenerator::PlaceSmallKiosk(int cx, int cy)
{
    if (!IsValid(cx - 2, cy - 2) || !IsValid(cx + 2, cy + 2)) return;

    // 주변 정리 (NONE으로 초기화)
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = EModelType::ROAD;

    // 가판대 본체 배치 (S자 모양 또는 한 줄)
    if (rand() % 2 == 0) {
        for (int y = cy - 1; y <= cy + 1; y++) mapGrid[y][cx] = EModelType::KIOSK;
    }
    else {
        for (int x = cx - 1; x <= cx + 1; x++) mapGrid[cy][x] = EModelType::KIOSK;
    }
}

// 대형 창고 배치
void MapGenerator::PlaceLargeWarehouse(int cx, int cy)
{
    if (!IsValid(cx - 3, cy - 3) || !IsValid(cx + 3, cy + 3)) return;

    // 주변 정리
    for (int y = cy - 3; y <= cy + 3; y++)
        for (int x = cx - 3; x <= cx + 3; x++) mapGrid[y][x] = EModelType::ROAD;

    // 창고 본체 배치 (W)
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = EModelType::WAREHOUSE;

    // 입구 생성 (창고 한 면의 중앙을 NONE으로 비움)
    int entranceDir = rand() % 4;
    if (entranceDir == 0) { // 위쪽 입구
        mapGrid[cy - 2][cx - 1] = EModelType::ROAD;
        mapGrid[cy - 2][cx] = EModelType::ROAD;
        mapGrid[cy - 2][cx + 1] = EModelType::ROAD;
    }
    else if (entranceDir == 1) { // 아래쪽 입구
        mapGrid[cy + 2][cx - 1] = EModelType::ROAD;
        mapGrid[cy + 2][cx] = EModelType::ROAD;
        mapGrid[cy + 2][cx + 1] = EModelType::ROAD;
    }
    else if (entranceDir == 2) { // 왼쪽 입구
        mapGrid[cy - 1][cx - 2] = EModelType::ROAD;
        mapGrid[cy][cx - 2] = EModelType::ROAD;
        mapGrid[cy + 1][cx - 2] = EModelType::ROAD;
    }
    else { // 오른쪽 입구
        mapGrid[cy - 1][cx + 2] = EModelType::ROAD;
        mapGrid[cy][cx + 2] = EModelType::ROAD;
        mapGrid[cy + 1][cx + 2] = EModelType::ROAD;
    }
}