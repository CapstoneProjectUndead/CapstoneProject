#include "pch.h"
#include "DBManager.h"

DBManager::DBManager()
	: driver(nullptr)
{

}

DBManager::~DBManager()
{

}

void DBManager::Init()
{
    //driver = get_driver_instance();
    //
    //const string server = "tcp://127.0.0.1:3306";
    //const string name = "root";
    //const string password = "projectuser~";
    //
    //// 데이터베이스에 연결합니다.
    //conn.reset(driver->connect(server, name, password));
    //
    //// 데이터베이스 작업을 수행합니다.
    //conn->setSchema("mydatabase");


    sql::Driver* driver = get_driver_instance();

    sql::ConnectOptionsMap props;
    props["hostName"] = "tcp://127.0.0.1:3306";
    props["userName"] = "root";
    props["password"] = "projectuser~";
    props["CLIENT_MULTI_STATEMENTS"] = true;

    // **가장 중요: UTF-8 사용 명시**
    props["characterEncoding"] = "utf8mb4";
    props["useUnicode"] = true;

    conn.reset(driver->connect(props));

    // 데이터베이스 작업을 수행합니다.
    conn->setSchema("mydatabase");

    // 그래도 혹시 모르니까 한번 더
    std::unique_ptr<sql::Statement> stmt(conn->createStatement());
    stmt->execute("SET NAMES utf8mb4");
}

std::string to_utf8(const std::string& ansi)
{
    // CP949 → UTF-16
    int wlen = MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_ACP, 0, ansi.c_str(), -1, &wstr[0], wlen);

    // UTF-16 → UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string u8str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &u8str[0], u8len, NULL, NULL);

    return u8str;
}

std::string to_ansi(const std::string& utf8)
{
    // UTF-8 → UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wstr[0], wlen);

    // UTF-16 → CP949 (ANSI)
    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string astr(alen, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &astr[0], alen, NULL, NULL);

    return astr;
}