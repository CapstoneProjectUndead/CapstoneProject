#pragma once
#include <ServerEngine/PacketUtils.h>

constexpr int PORT_NUM = 7777;
constexpr int ID_SIZE = 30;
constexpr int PW_SIZE = 30;
constexpr int NAME_SIZE = 20;
constexpr int CHAT_SIZE = 100;

// Packet ID
// 아래는 예시
// 우리 게임에 맞게 변형
constexpr int16 _C_SIGNUP = 0;
constexpr int16 _S_SIGNRES = 1;
constexpr int16 _C_LOGIN = 2;
constexpr int16 _S_LOGIN = 3;
constexpr int16 _S_LOGIN_FAIL = 4;
constexpr int16 _S_USERLIST = 5;

#pragma pack (push, 1)



#pragma pack (pop)