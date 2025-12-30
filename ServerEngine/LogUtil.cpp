#include "pch.h"
#include "LogUtil.h"

void LogUtil::error_display(int err_no)
{
    WCHAR* lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, err_no,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, 0);
    std::wcout << lpMsgBuf << std::endl;
    //while (true);
    LocalFree(lpMsgBuf);
}

void LogUtil::error_display(const char* msg)
{
    WCHAR* lpMsgBuf;
    int ErrorNum = WSAGetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, ErrorNum,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    std::cout << msg << std::endl;;
    std::wcout << "[Error Num: " << ErrorNum << "]" << lpMsgBuf << std::endl;
    //while (true);
    // µð¹ö±ë ¿ë
    LocalFree(lpMsgBuf);
}

void LogUtil::PrintLog(const char* s)
{
#ifdef _DEBUG
    cout << s << endl;
#endif // _DEBUG
}