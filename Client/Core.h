#ifdef _DEBUG
#pragma comment(lib, "ServerEngine\\ServerEngine_d.lib")
#else
#pragma comment(lib, "ServerEngine\\ServerEngine.lib")
#endif

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <array>
//#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <mutex>
#include <atomic>
#include <thread>

#include <windows.h>
#include <iostream>
#include <locale>
#include <assert.h>
//#include <functional>
#include <numeric>
#include <memory>