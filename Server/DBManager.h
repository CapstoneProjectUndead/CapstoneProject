#pragma once
#include <MySQL/jdbc/mysql_connection.h>
#include <MySQL/jdbc/cppconn/driver.h>
#include <MySQL/jdbc/cppconn/exception.h>
#include <MySQL/jdbc/cppconn/prepared_statement.h>

class DBManager
{
private:
    DBManager();
    DBManager(const DBManager&) = delete;

public:
    ~DBManager();

    static DBManager& GetInstance() {
        static DBManager instance;
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