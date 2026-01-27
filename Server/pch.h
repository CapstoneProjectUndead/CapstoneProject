// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

// Server

#pragma once

#ifndef PCH_H
#define PCH_H

#ifdef _DEBUG
#pragma comment(lib, "ServerEngine\\ServerEngine_d.lib")
#else
#pragma comment(lib, "ServerEngine\\ServerEngine.lib")
#endif


// 여기에 미리 컴파일하려는 헤더 추가
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

#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

#include <windows.h>
#include <iostream>
#include <locale>
#include <assert.h>
#include <functional>
#include <algorithm>

using std::cout;
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
using std::static_pointer_cast;

using std::shared_ptr;
using std::weak_ptr;
using std::unique_ptr;
using std::make_shared;
using std::make_unique;
using std::make_pair;

#include <ServerEngine/global.h>
#include <ServerEngine/ThreadManager.h>

#include <ServerEngine/Session.h>
#include <ServerEngine/Service.h>
#include <ServerEngine/SocketHelper.h>
#include <ServerEngine/SendBuffer.h>
#include <ServerEngine/BufferWriter.h>
#include <ServerEngine/BufferReader.h>

#include <enum.h>
#include <struct.h>
#include <protocol.h>
#include <VarialbePacketWriter.h>

#include "ClientPacketHandler.h"
#include "DirectXMathHelper.h"

extern unique_ptr<class CGameFramework> gGameFramework;
//extern DBConnector g_db;

extern const double g_server_targetTick;
extern const double g_targetDT;


#endif //PCH_H

