#include "pch.h"
#include "DBManager.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <mutex>

bool g_db_connected = false;

CDBManager::CDBManager()
	: driver(nullptr)
{

}

CDBManager::~CDBManager()
{

}

void CDBManager::Init()
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


    try {
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

        // 여기까지 예외 없이 도달 = 연결 성공
        g_db_connected = true;
        std::cout << "[DB] MySQL 연결 성공\n";
    }
    catch (sql::SQLException& e) {
        // 연결 실패: 서버는 죽지 않고 파일 모드로 동작한다.
        g_db_connected = false;
        conn.reset();
        std::cout << "[DB] MySQL 연결 실패 - 파일(users.json) 모드로 동작\n";
        std::cout << "  Error: " << e.what() << "\n";
        std::cout << "  Code:  " << e.getErrorCode() << "\n";
        std::cout << "  State: " << e.getSQLStateCStr() << "\n";
    }
}

// 회원 백업 파일 경로 (저장/읽기 공용).
static std::filesystem::path GetUsersFilePath()
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // <repo>\x64\<Config>\Server.exe → parent 3번 = <repo>
    std::filesystem::path repoRoot =
        std::filesystem::path(exePath).parent_path().parent_path().parent_path();

    return repoRoot / "External" / "Common" / "Data" / "users.json";
}

// 저장/읽기가 같은 파일을 만지므로 하나의 mutex로 직렬화
static std::mutex g_users_file_mtx;

bool SaveUserToFile(const std::string& id, const std::string& pw, const std::string& name)
{
    // 여러 세션(스레드)에서 동시에 가입할 수 있으므로 파일 접근을 직렬화
    std::lock_guard<std::mutex> lg(g_users_file_mtx);

    // to_utf8 결과 끝에 붙는 널 문자 제거 (JSON 파일 오염 방지)
    auto trimNull = [](std::string s) {
        size_t pos = s.find('\0');
        if (pos != std::string::npos) s.resize(pos);
        return s;
    };
    const std::string sid   = trimNull(id);
    const std::string spw   = trimNull(pw);
    const std::string sname = trimNull(name);

    const std::filesystem::path path = GetUsersFilePath();

    // 기존 파일 로드 (없거나 깨졌으면 빈 배열로 시작)
    nlohmann::json arr = nlohmann::json::array();
    {
        std::ifstream in(path);
        if (in.is_open()) {
            try { in >> arr; }
            catch (...) { arr = nlohmann::json::array(); }
            if (!arr.is_array()) arr = nlohmann::json::array();
        }
    }

    // 중복 id 검사 (이미 있으면 저장하지 않음)
    for (const auto& e : arr) {
        if (e.value("id", std::string{}) == sid) {
            return false;
        }
    }

    // 새 유저 추가
    nlohmann::json user;
    user["id"]       = sid;
    user["password"] = spw;
    user["name"]     = sname;
    arr.push_back(user);

    // Data 폴더가 없으면 생성 후 저장
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path, std::ios::trunc);
    if (!out.is_open()) {
        std::cout << "[File] users.json 저장 실패: " << path.string() << "\n";
        return false;
    }
    out << arr.dump(4);   // 사람이 보기 좋게 들여쓰기 저장 (UTF-8)
    return true;
}

bool LoadUserFromFile(const std::string& id, std::string& outPw, std::string& outName)
{
    std::lock_guard<std::mutex> lg(g_users_file_mtx);

    // 입력 id 끝에 널문자가 붙어있을 수 있으므로 제거 (파일은 trim된 값으로 저장됨)
    std::string sid = id;
    {
        size_t pos = sid.find('\0');
        if (pos != std::string::npos) sid.resize(pos);
    }

    const std::filesystem::path path = GetUsersFilePath();

    std::ifstream in(path);
    if (!in.is_open()) return false;

    nlohmann::json arr = nlohmann::json::array();
    try { in >> arr; }
    catch (...) { return false; }
    if (!arr.is_array()) return false;

    // id로 유저 검색
    for (const auto& e : arr) {
        if (e.value("id", std::string{}) == sid) {
            outPw   = e.value("password", std::string{});
            outName = e.value("name", std::string{});
            return true;
        }
    }
    return false;
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