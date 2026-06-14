#include "pch.h"
// 서버쪽 TitleScene
#include "TitleScene.h"
#include "Player.h"
#include "RoomManager.h"

#undef min
#undef max


CTitleScene::CTitleScene()
	: CScene(SCENE_TYPE::TITLE)
{

}

CTitleScene::~CTitleScene()
{

}

void CTitleScene::Initialize()
{
	CScene::Initialize();
}

void CTitleScene::Update(float elapsedTime)
{
	CScene::Update(elapsedTime);
}

void CTitleScene::EnterUser(shared_ptr<CUser> user)
{
	lock_guard<mutex> lg(users_lock);
	users[user->GetUserID()] = user;
}

void CTitleScene::LeaveUser(uint64 id)
{
	lock_guard<mutex> lg(users_lock);
	users.erase(id);
}

void CTitleScene::Handle_C_SignUp(shared_ptr<Session> session, const C_SIGNUP& pkt)
{
	string id = to_utf8(pkt.id);
	string pw = to_utf8(pkt.password);
	string username = to_utf8(pkt.name);

	if ((id.size() == 1) || (pw.size() == 1)) {
		S_SIGN_RES failPkt;
		failPkt.success = false;
		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
		session->DoSend(sendBuffer);

		return;
	}

	bool success = false;

	// DB가 연결돼 있을 때만 DB에 저장 (id가 PK라 중복이면 예외 → 실패)
	if (g_db_connected) {
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(CON->prepareStatement(
				"INSERT INTO users (id, password, name) VALUES (?, ?, ?)"
			));

			pstmt->setString(1, id);
			pstmt->setString(2, pw);
			pstmt->setString(3, username);

			// INSERT는 executeUpdate() 사용
			int affected = pstmt->executeUpdate();
			success = (affected > 0);
		}
		catch (sql::SQLException& e) {
			// 중복 아이디 등으로 INSERT 실패
			cout << "[DB] SQL 예외 발생 (회원가입)\n";
			cout << "  Error: " << e.what() << "\n";
			cout << "  Code:  " << e.getErrorCode() << "\n";
			cout << "  State: " << e.getSQLStateCStr() << "\n";
			success = false;
		}
	}

	// DB 연결 여부와 무관하게 항상 파일(users.json)에도 저장한다.
	// (이미 같은 id가 있으면 내부에서 건너뛰고 false 반환)
	bool fileSaved = SaveUserToFile(id, pw, username);

	// DB가 꺼져 있을 땐 파일 저장 성공 여부를 가입 성공 기준으로 사용
	if (!g_db_connected) {
		success = fileSaved;
	}

	S_SIGN_RES resPkt;
	resPkt.success = success;
	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(resPkt);
	session->DoSend(sendBuffer);
}

void CTitleScene::Handle_C_LogIn(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// guest 유저 로그인 처리
	if (pkt.guest_login) {
		ProcessLogin(session, true, string{});
		return;
	}

	// 클라가 보낸 id/password (클라는 CP949(ANSI)로 전송 → 비교용 UTF-8로 변환)
	string inputId = to_utf8(pkt.id);
	string inputPw = to_utf8(pkt.password);

	// 로그인 실패 응답 공통 처리
	auto SendLoginFail = [&]() {
		S_LOGIN failPkt;
		failPkt.success = false;
		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
		session->DoSend(sendBuffer);
	};

	if (g_db_connected) {
		// === DB 모드: MySQL에서 조회 ===
		try {
			std::unique_ptr<sql::PreparedStatement> pstmt(CON->prepareStatement(
				"SELECT id, password, name FROM users WHERE id = ?"
			));
			pstmt->setString(1, inputId);

			std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

			if (res->next()) {
				// row 존재 → 해당 아이디가 DB에 있음
				string pw = res->getString("password");
				string name = res->getString("name");

				if (inputPw == pw) {
					ProcessLogin(session, false, name);   // 로그인 성공
					// 저장/로드 키로 쓸 계정 ID 보관 (끝 널문자 제거)
					CAST_CS(session)->GetUser()->SetAccountId(inputId.c_str());
				}
				else {
					SendLoginFail();                       // 비밀번호 불일치
				}
			}
			else {
				SendLoginFail();                           // 아이디 없음
			}
		}
		catch (sql::SQLException& e) {
			cout << "[DB] SQL 예외 발생 (로그인)\n";
			cout << "  Error: " << e.what() << "\n";
			cout << "  Code:  " << e.getErrorCode() << "\n";
			cout << "  State: " << e.getSQLStateCStr() << "\n";
			SendLoginFail();
		}
	}
	else {
		// === 파일 모드: DB 미연결 시 users.json에서 조회 ===
		// 파일은 널문자를 제거하고 저장하므로, 비교용 입력값도 널문자를 제거한다.
		// (c_str()은 첫 널문자까지만 반환 → 트림 효과)
		string fileId = inputId.c_str();
		string filePw = inputPw.c_str();

		string storedPw, storedName;
		if (LoadUserFromFile(fileId, storedPw, storedName) && filePw == storedPw) {
			ProcessLogin(session, false, storedName);     // 로그인 성공
			// 저장/로드 키로 쓸 계정 ID 보관 (fileId는 이미 트림됨)
			CAST_CS(session)->GetUser()->SetAccountId(fileId);
		}
		else {
			SendLoginFail();                               // 아이디 없음 또는 비번 불일치
		}
	}
}

void CTitleScene::Handle_C_LogOut(shared_ptr<Session> session, const C_LOGOUT& pkt)
{
	LeaveUser(pkt.user_id);
	CAST_CS(session)->SetUser(nullptr);

	S_LOGOUT logOutPkt;
	logOutPkt.success = true;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_LOGOUT>(logOutPkt);
	session->DoSend(sendBuffer);
}

void CTitleScene::ProcessLogin(shared_ptr<Session> session, bool guest, string name)
{
	// CUser 생성 (생성자 안에서 ID발급)
	shared_ptr<CUser> user = make_shared<CUser>();

	if (guest) {
		// "player" + 숫자(ID)를 문자열로 결합. to_string 없이 더하면 포인터 연산이 되어 버그.
		name = "player" + std::to_string(user->GetUserID());
		user->SetGuest(true);
	}

	// 유저 이름 부여 (화면 표시용은 다시 CP949로)
	user->SetName(to_ansi(name));

	// session이 유저를 들고있는다. RefCount 증가
	CAST_CS(session)->SetUser(user);

	// 유저도 자신의 세션을 들고 있는다. (약한 참조. RefCount 증가 x)
	user->SetSession(session);

	// CUser 컨테이너에 저장
	EnterUser(user);

	// 클라한테 ID와 함께 로그인 허락 패킷을 보낸다. (S_LOGIN)
	S_LOGIN loginPkt;
	loginPkt.success = true;
	loginPkt.user_id = user->GetUserID();
	COPY_STRING(loginPkt.name, user->GetName().c_str());   // 본인 이름 통보
	SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(loginPkt);
	session->DoSend(sendBuffer);

	// 로그인 성공한 유저는 룸 매칭 화면으로 가게된다.
	// 현재 입장 가능한 방 목록을 알려준다.
	CRoomManager::GetInstance().SendRoomList(session);
}
