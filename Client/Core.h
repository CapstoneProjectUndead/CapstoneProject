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
#include <vector>
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
#include <functional>
#include <numeric>

using std::cout;
using std::cin;
using std::endl;
using std::wcout;
using std::locale;
using std::string;
using std::wstring;
using std::array;
using std::vector;
using std::list;
using std::queue;
using std::stack;
using std::map;
using std::set;
using std::unordered_map;
using std::unordered_set;
using std::mutex;
using std::atomic;
using std::thread;
using std::function;
using std::lock_guard;
using std::enable_shared_from_this;

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_pair;