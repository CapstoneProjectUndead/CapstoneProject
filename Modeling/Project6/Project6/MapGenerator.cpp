#include "MapGenerator.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <stack>

using namespace std;
using namespace DirectX;

// ====================================================================
// 🛠️ [도우미 구역] 익명 네임스페이스
// ====================================================================
namespace {
    const int WIDTH = 101;  // 1칸 = 1m (가로 100m)
    const int HEIGHT = 201; // 1칸 = 1m (세로 200m)
    char mapGrid[HEIGHT][WIDTH];

    int dx[] = { 0, 0, -2, 2 };
    int dy[] = { -2, 2, 0, 0 };

    bool IsValid(int x, int y) {
        return (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1);
    }

    void CarveMaze(int startX, int startY) {
        struct Cell { int x, y; };
        std::stack<Cell> s;

        s.push({ startX, startY });
        mapGrid[startY][startX] = ' ';

        while (!s.empty()) {
            Cell current = s.top();
            int x = current.x;
            int y = current.y;

            int dirs[] = { 0, 1, 2, 3 };
            for (int i = 0; i < 4; i++) swap(dirs[i], dirs[rand() % 4]);

            bool moved = false;
            for (int i = 0; i < 4; i++) {
                int nx = x + dx[dirs[i]];
                int ny = y + dy[dirs[i]];

                if (IsValid(nx, ny) && mapGrid[ny][nx] == '#') {
                    mapGrid[y + dy[dirs[i]] / 2][x + dx[dirs[i]] / 2] = ' ';
                    mapGrid[ny][nx] = ' ';
                    s.push({ nx, ny });
                    moved = true;
                    break;
                }
            }
            if (!moved) {
                s.pop();
            }
        }
    }

    // 🌟 [새로 추가된 마법!] 공터의 영역(네모)을 기억할 수첩(구조체)
    struct Rect {
        int x, y, w, h;
        // 다른 네모랑 겹치는지 확인하는 똑똑한 함수 (1칸씩 여유를 둬서 딱 붙지도 않게 해!)
        bool Intersects(const Rect& other) const {
            return !(x + w + 1 <= other.x || x - 1 >= other.x + other.w ||
                y + h + 1 <= other.y || y - 1 >= other.y + other.h);
        }
    };

    // 🔥 [업그레이드된 마법!] 안 겹치게 파내는 똑똑한 포크레인!
    void CreateOpenSpaces(int numSpaces) {
        std::vector<Rect> builtSpaces; // 여기가 바로 '수첩'이야!
        int attempts = 0; // 자리를 너무 못 찾아서 무한루프에 빠지지 않게 횟수 제한을 둠

        // 20개를 다 짓거나, 자리를 1000번 넘게 찾아봤는데도 없으면 그만두기
        for (int i = 0; i < numSpaces && attempts < 1000; ) {
            int w, h;
            int sizeType = rand() % 3;

            if (sizeType == 0) {
                w = 4 + rand() % 4; h = 4 + rand() % 4; // 소형
            }
            else if (sizeType == 1) {
                w = 8 + rand() % 5; h = 8 + rand() % 5; // 중형
            }
            else {
                w = 13 + rand() % 6; h = 13 + rand() % 6; // 대형
            }

            int startX = 2 + rand() % (WIDTH - w - 2);
            int startY = 2 + rand() % (HEIGHT - h - 2);

            Rect newSpace = { startX, startY, w, h };
            bool overlap = false;

            // 수첩을 펼쳐서 겹치는 곳이 있는지 하나씩 확인!
            for (const auto& existing : builtSpaces) {
                if (newSpace.Intersects(existing)) {
                    overlap = true; // 앗! 겹친다!
                    break;
                }
            }

            // 안 겹치면(overlap이 false면) 진짜로 땅을 팝니다!
            if (!overlap) {
                for (int y = startY; y < startY + h; y++) {
                    for (int x = startX; x < startX + w; x++) {
                        mapGrid[y][x] = ' ';
                    }
                }
                builtSpaces.push_back(newSpace); // 수첩에 방금 판 곳을 기록!
                i++; // 성공적으로 1개 지었으니 숫자 증가!
            }
            attempts++; // 자리 찾기 시도 횟수 증가
        }
    }

    void PlaceSmallKiosk(int cx, int cy) {
        if (cx - 2 <= 0 || cx + 2 >= WIDTH - 1 || cy - 2 <= 0 || cy + 2 >= HEIGHT - 1) return;
        for (int y = cy - 2; y <= cy + 2; y++)
            for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = ' ';

        if (rand() % 2 == 0) {
            for (int y = cy - 1; y <= cy + 1; y++) mapGrid[y][cx] = 'S';
        }
        else {
            for (int x = cx - 1; x <= cx + 1; x++) mapGrid[cy][x] = 'S';
        }
    }

    void PlaceMediumStore(int cx, int cy) {
        if (cx - 2 <= 0 || cx + 2 >= WIDTH - 1 || cy - 2 <= 0 || cy + 2 >= HEIGHT - 1) return;
        for (int y = cy - 2; y <= cy + 2; y++)
            for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = ' ';
        for (int y = cy - 1; y <= cy + 1; y++)
            for (int x = cx - 1; x <= cx + 1; x++) mapGrid[y][x] = 'M';

        int doorDir = rand() % 4;
        if (doorDir == 0) mapGrid[cy - 1][cx] = 'd';
        else if (doorDir == 1) mapGrid[cy + 1][cx] = 'd';
        else if (doorDir == 2) mapGrid[cy][cx - 1] = 'd';
        else mapGrid[cy][cx + 1] = 'd';
    }

    void PlaceLargeWarehouse(int cx, int cy) {
        if (cx - 3 <= 0 || cx + 3 >= WIDTH - 1 || cy - 3 <= 0 || cy + 3 >= HEIGHT - 1) return;
        for (int y = cy - 3; y <= cy + 3; y++)
            for (int x = cx - 3; x <= cx + 3; x++) mapGrid[y][x] = ' ';
        for (int y = cy - 2; y <= cy + 2; y++)
            for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = 'W';

        int entranceDir = rand() % 4;
        if (entranceDir == 0) { mapGrid[cy - 2][cx - 1] = ' '; mapGrid[cy - 2][cx] = ' '; mapGrid[cy - 2][cx + 1] = ' '; }
        else if (entranceDir == 1) { mapGrid[cy + 2][cx - 1] = ' '; mapGrid[cy + 2][cx] = ' '; mapGrid[cy + 2][cx + 1] = ' '; }
        else if (entranceDir == 2) { mapGrid[cy - 1][cx - 2] = ' '; mapGrid[cy][cx - 2] = ' '; mapGrid[cy + 1][cx - 2] = ' '; }
        else { mapGrid[cy - 1][cx + 2] = ' '; mapGrid[cy][cx + 2] = ' '; mapGrid[cy + 1][cx + 2] = ' '; }
    }

    void PlaceParkPlaza(int cx, int cy) {
        if (cx - 2 <= 0 || cx + 2 >= WIDTH - 1 || cy - 2 <= 0 || cy + 2 >= HEIGHT - 1) return;
        for (int y = cy - 2; y <= cy + 2; y++)
            for (int x = cx - 2; x <= cx + 2; x++) mapGrid[y][x] = ' ';
        mapGrid[cy][cx] = 'n';
        mapGrid[cy][cx - 1] = 't';
        mapGrid[cy][cx + 1] = 't';
    }
} // 네임스페이스 끝!


// ====================================================================
// 🚀 [메인 구역] 외부에서 호출하는 진짜 맵 생성기 함수
// ====================================================================
std::vector<InstanceData> MapGenerator::Generate3DMap() {
    std::vector<InstanceData> instanceList;
    srand((unsigned int)time(NULL));

    // [1단계 ~ 4단계] 2D 미로 생성 및 구역(Zone) 나누기
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) mapGrid[y][x] = '#';

    CarveMaze(1, 1);

    for (int i = 0; i < (WIDTH * HEIGHT) / 3; i++) {
        mapGrid[1 + rand() % (HEIGHT - 2)][1 + rand() % (WIDTH - 2)] = ' ';
    }

    // 🔥 [스마트 포크레인 출동!] 겹치지 않게 예쁜 공터 20개 생성!
    CreateOpenSpaces(50);

    // Y < 100 은 공원(T), 그 위는 상점가(#)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mapGrid[y][x] == '#') {
                if (y < 100) mapGrid[y][x] = 'T';
            }
        }
    }

    // [5단계] 맵 크기(100x200)에 맞춰 건물과 광장 스폰
    int numWarehouse = 2 + rand() % 3;
    for (int i = 0; i < numWarehouse; i++)
        PlaceLargeWarehouse(4 + rand() % (WIDTH - 8), 130 + rand() % 60);

    int numStore = 8 + rand() % 5;
    for (int i = 0; i < numStore; i++)
        PlaceMediumStore(4 + rand() % (WIDTH - 8), 100 + rand() % 90);

    int numKiosk = 20 + rand() % 10;
    for (int i = 0; i < numKiosk; i++)
        PlaceSmallKiosk(4 + rand() % (WIDTH - 8), 100 + rand() % 90);

    int numPark = 20 + rand() % 5;
    for (int i = 0; i < numPark; i++)
        PlaceParkPlaza(3 + rand() % (WIDTH - 6), 5 + rand() % 90);

    // [6단계] 보물($) 배치
    const int BLOCK_SIZE = 10;
    for (int by = 0; by < HEIGHT; by += BLOCK_SIZE) {
        for (int bx = 0; bx < WIDTH; bx += BLOCK_SIZE) {
            int treasureCount = rand() % 4;
            vector<pair<int, int>> emptySpaces;
            for (int y = by; y < by + BLOCK_SIZE && y < HEIGHT; y++) {
                for (int x = bx; x < bx + BLOCK_SIZE && x < WIDTH; x++) {
                    char c = mapGrid[y][x];
                    if (c == ' ' || c == 'S' || c == 'M' || c == 'W') {
                        emptySpaces.push_back({ x, y });
                    }
                }
            }
            for (int t = 0; t < treasureCount && !emptySpaces.empty(); t++) {
                int randIdx = rand() % emptySpaces.size();
                mapGrid[emptySpaces[randIdx].second][emptySpaces[randIdx].first] = '$';
                emptySpaces.erase(emptySpaces.begin() + randIdx);
            }
        }
    }

    // ====================================================================
    // 🌟 [7단계] 완성된 2D 배열을 3D 큐브(InstanceData) 리스트로 변환!
    // ====================================================================
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            char c = mapGrid[y][x];

            if (c == ' ') continue;

            InstanceData inst;
            float posX = (float)x * 1.0f;
            float posZ = (float)y * 1.0f;

            if (c == '#') {
                inst.color = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
            }
            else if (c == 'W') {
                inst.color = XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 6.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 3.0f, posZ);
            }
            else if (c == 'M') {
                inst.color = XMFLOAT4(0.2f, 0.6f, 0.8f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
            }
            else if (c == 'S') {
                inst.color = XMFLOAT4(0.2f, 0.2f, 0.8f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 2.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.0f, posZ);
            }
            else if (c == 'T') {
                inst.color = XMFLOAT4(0.1f, 0.5f, 0.1f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 1.5f, 1.0f);
                inst.position = XMFLOAT3(posX, 0.75f, posZ);
            }
            else if (c == '$') {
                inst.color = XMFLOAT4(1.0f, 0.8f, 0.0f, 1.0f);
                inst.scale = XMFLOAT3(0.5f, 0.5f, 0.5f);
                inst.position = XMFLOAT3(posX, 0.25f, posZ);
            }
            else {
                inst.color = XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 0.5f, posZ);
            }

            instanceList.push_back(inst);
        }
    }

    return instanceList;
}