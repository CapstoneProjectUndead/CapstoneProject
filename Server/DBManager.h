#pragma once
#include <MySQL/jdbc/mysql_connection.h>
#include <MySQL/jdbc/cppconn/driver.h>
#include <MySQL/jdbc/cppconn/exception.h>
#include <MySQL/jdbc/cppconn/prepared_statement.h>

extern bool g_db_connected;

class CDBManager
{
private:
    CDBManager();
    CDBManager(const CDBManager&) = delete;

public:
    ~CDBManager();

    static CDBManager& GetInstance() {
        static CDBManager instance;
        return instance;
    }

public:
	void Init();

	std::unique_ptr<sql::Connection>& GetCon() { return conn; }

private:
	sql::Driver* driver;
	std::unique_ptr<sql::Connection> conn;
};

std::string to_utf8(const std::string& ansi);
std::string to_ansi(const std::string& utf8);

// 회원 정보를 파일(Data/users.json)에 저장한다.
// 이미 같은 id가 있으면 저장하지 않고 false, 새로 저장하면 true를 반환한다.
// (인자는 UTF-8 문자열을 받는다. DB 연결 여부와 무관하게 항상 호출하는 백업 저장소)
bool SaveUserToFile(const std::string& id, const std::string& pw, const std::string& name);

// 파일(Data/users.json)에서 id로 유저를 찾는다. (DB 미연결 시 로그인용)
// 찾으면 outPw/outName에 저장값을 채우고 true, 없으면 false.
bool LoadUserFromFile(const std::string& id, std::string& outPw, std::string& outName);