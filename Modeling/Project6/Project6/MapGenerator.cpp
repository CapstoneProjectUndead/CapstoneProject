#include "MapGenerator.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <stack>
#include <iostream>

using namespace std;
using namespace DirectX;

// ====================================================================
// 🛠️ [설정 구역] 여기서 맵 크기를 마음대로 바꿔보세요!
// ====================================================================
namespace {
    // 🔥 [자동화의 핵심!] 여기 숫자만 바꾸면 맵 전체가 알아서 맞춰집니다. (홀수 추천)
    const int WIDTH = 51;   // 가로 크기
    const int HEIGHT = 101; // 세로 크기

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

    struct Rect {
        int x, y, w, h;
        bool Intersects(const Rect& other) const {
            return !(x + w + 1 <= other.x || x - 1 >= other.x + other.w ||
                y + h + 1 <= other.y || y - 1 >= other.y + other.h);
        }
    };

    void CreateOpenSpaces(int numSpaces) {
        std::vector<Rect> builtSpaces;
        int attempts = 0;

        for (int i = 0; i < numSpaces && attempts < 1000; ) {
            int w, h;
            int sizeType = rand() % 3;

            // 공터 크기도 맵 크기에 맞춰서 최대치가 넘지 않게 안전장치 추가!
            if (sizeType == 0) { w = 3 + rand() % 4; h = 3 + rand() % 4; }
            else if (sizeType == 1) { w = 7 + rand() % 5; h = 7 + rand() % 5; }
            else { w = 12 + rand() % 5; h = 12 + rand() % 5; }

            w = min(w, WIDTH - 6); // 맵보다 큰 공터가 생기지 않게 방어
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
                for (int y = startY; y < startY + h; y++) {
                    for (int x = startX; x < startX + w; x++) {
                        mapGrid[y][x] = ' ';
                    }
                }
                builtSpaces.push_back(newSpace);
                i++;
            }
            attempts++;
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
// 🚀 [메인 구역] 맵 데이터 생성
// ====================================================================
std::vector<InstanceData> MapGenerator::Generate3DMap() {
    std::vector<InstanceData> instanceList;
    srand((unsigned int)time(NULL));

    // 🔥 [자동 계산 로직] 맵 넓이에 맞춰서 생성될 개수 계산하기!
    float areaRatio = (WIDTH * HEIGHT) / 5000.0f; // 50x100 맵을 기준(1.0)으로 잡음
    int halfHeight = HEIGHT / 2; // 맵의 절반 (아래는 공원, 위는 상점가)

    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) mapGrid[y][x] = '#';

    CarveMaze(1, 1);

    for (int i = 0; i < (WIDTH * HEIGHT) / 3; i++) {
        mapGrid[1 + rand() % (HEIGHT - 2)][1 + rand() % (WIDTH - 2)] = ' ';
    }

    // 면적에 비례해서 공터 개수 조절
    CreateOpenSpaces(max(5, (int)(20 * areaRatio)));

    // 절반(halfHeight) 이하는 공원(T), 그 위는 상점가(#)
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mapGrid[y][x] == '#') {
                if (y < halfHeight) mapGrid[y][x] = 'T';
            }
        }
    }

    // ====================================================================
    // 🏢 건물 자동 스폰 (위치도 맵 크기에 비례해서 자동 계산!)
    // ====================================================================
    int numWarehouse = max(1, (int)(3 * areaRatio));
    for (int i = 0; i < numWarehouse; i++) {
        int rX = 4 + rand() % (WIDTH - 8);
        int rY = halfHeight + rand() % (halfHeight - 10); // 무조건 상점가 구역(위쪽)
        PlaceLargeWarehouse(rX, rY);
    }

    int numStore = max(2, (int)(10 * areaRatio));
    for (int i = 0; i < numStore; i++) {
        int rX = 4 + rand() % (WIDTH - 8);
        int rY = halfHeight + rand() % (halfHeight - 5);
        PlaceMediumStore(rX, rY);
    }

    int numKiosk = max(5, (int)(25 * areaRatio));
    for (int i = 0; i < numKiosk; i++) {
        int rX = 4 + rand() % (WIDTH - 8);
        int rY = halfHeight + rand() % (halfHeight - 5);
        PlaceSmallKiosk(rX, rY);
    }

    int numPark = max(5, (int)(20 * areaRatio));
    for (int i = 0; i < numPark; i++) {
        int rX = 3 + rand() % (WIDTH - 6);
        int rY = 5 + rand() % (halfHeight - 10); // 무조건 공원 구역(아래쪽)
        PlaceParkPlaza(rX, rY);
    }

    // 보물($) 배치
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
    // 콘솔창 2D 맵 출력
    // ====================================================================
    cout << "\n================ 🗺️ 완성된 2D 텍스트 미니맵 (" << WIDTH << "x" << HEIGHT << ") =================\n";
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (mapGrid[y][x] == ' ') cout << ' ';
            else cout << mapGrid[y][x];
        }
        cout << "\n";
    }
    cout << "=================================================================\n\n";

    // ====================================================================
    // 🌟 [7단계] 3D 디테일 업그레이드! (벤치, 문, 나무 등등 귀여운 사이즈 적용)
    // ====================================================================
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            char c = mapGrid[y][x];

            if (c == ' ') continue;

            InstanceData inst;
            float posX = (float)x * 1.0f;
            float posZ = (float)y * 1.0f;

            if (c == '#') { // 골목길 벽
                inst.color = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
            }
            else if (c == 'W') { // 대형 창고
                inst.color = XMFLOAT4(0.8f, 0.1f, 0.1f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 6.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 3.0f, posZ);
            }
            else if (c == 'M') { // 중형 상점
                inst.color = XMFLOAT4(0.2f, 0.6f, 0.8f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 3.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.5f, posZ);
            }
            else if (c == 'S') { // 소형 가판대
                inst.color = XMFLOAT4(0.2f, 0.2f, 0.8f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 2.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 1.0f, posZ);
            }
            else if (c == 'T' || c == 't') { // 덤불 및 공원 나무
                inst.color = XMFLOAT4(0.1f, 0.6f, 0.2f, 1.0f);
                inst.scale = XMFLOAT3(0.8f, 1.5f, 0.8f); // 약간 슬림한 나무!       
                inst.position = XMFLOAT3(posX, 0.75f, posZ);
            }
            else if (c == '$') { // 보물 상자
                inst.color = XMFLOAT4(1.0f, 0.8f, 0.0f, 1.0f);
                inst.scale = XMFLOAT3(0.5f, 0.5f, 0.5f); // 50cm 귀요미!       
                inst.position = XMFLOAT3(posX, 0.25f, posZ);
            }
            else if (c == 'n') { // 공원 벤치
                inst.color = XMFLOAT4(0.5f, 0.3f, 0.1f, 1.0f); // 나무 색
                inst.scale = XMFLOAT3(0.8f, 0.4f, 0.4f);       // 가로로 길고 낮은 벤치
                inst.position = XMFLOAT3(posX, 0.2f, posZ);
            }
            else if (c == 'd') { // 상점 문
                inst.color = XMFLOAT4(0.3f, 0.15f, 0.05f, 1.0f); // 진한 나무 문
                inst.scale = XMFLOAT3(0.8f, 2.0f, 0.2f);       // 얇고 긴 문!
                inst.position = XMFLOAT3(posX, 1.0f, posZ);
            }
            else { // 혹시 모를 나머지 기본
                inst.color = XMFLOAT4(0.6f, 0.3f, 0.1f, 1.0f);
                inst.scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
                inst.position = XMFLOAT3(posX, 0.5f, posZ);
            }

            instanceList.push_back(inst);
        }
    }

    return instanceList;
}