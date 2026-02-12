#include "pch.h"
// 서버쪽 TitleScene
#include "TitleScene.h"
#include "ClientSession.h"
#include "Player.h"
#include "RoomManager.h"

#undef min
#undef max

#define CAST_CS(session) static_pointer_cast<CClientSession>(session)


CTitleScene::CTitleScene()
	: CScene(SCENE_TYPE::TITLE)
{

}

CTitleScene::~CTitleScene()
{

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

void CTitleScene::HandleSignUp(shared_ptr<Session> session, const C_SIGNUP& pkt)
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

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(CON->prepareStatement(
			"INSERT INTO users (id, password, name) VALUES (?, ?, ?)"
		));

		pstmt->setString(1, id);
		pstmt->setString(2, pw);
		pstmt->setString(3, username);

		// INSERT는 executeUpdate() 사용
		int affected = pstmt->executeUpdate();

		if (affected == 0)
		{
			// INSERT 됐어야 하는데, 0행 영향 → 비정상
			cout << "[DB] INSERT 실패: 0행 영향\n";

			{
				S_SIGN_RES failPkt;
				failPkt.success = false;
				SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
				session->DoSend(sendBuffer);
			}

			return;
		}
		// 회원 가입 성공
		else {
			S_SIGN_RES successPkt;
			successPkt.success = true;
			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(successPkt);
			session->DoSend(sendBuffer);
		}

	}
	catch (sql::SQLException& e)
	{
		cout << "[DB] SQL 예외 발생\n";
		cout << "  Error: " << e.what() << "\n";
		cout << "  Code:  " << e.getErrorCode() << "\n";
		cout << "  State: " << e.getSQLStateCStr() << "\n";

		{
			S_SIGN_RES failPkt;
			failPkt.success = false;
			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
			session->DoSend(sendBuffer);
		}

		return;  // DB 에러 처리
	}
}

void CTitleScene::HandleLogIn(shared_ptr<Session> session, const C_LOGIN& pkt)
{
	// 유저 로그인 처리
	{
		// CUser 생성 (생성자 안에서 ID발급)
		shared_ptr<CUser> user = make_shared<CUser>();

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
		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(loginPkt);
		session->DoSend(sendBuffer);
	}

	// 지금 로그인한 유저는 룸 매칭 화면으로 가게된다.
	// 그러면 현재 입장 가능한 방 목록을 알려줘야한다.
	{
		const auto& rooms = CRoomManager::GetInstance().GetRooms();
		if (!rooms.empty()) {

			int32 roomCount = rooms.size();
			int32 pktSize = sizeof(S_Room_List) + sizeof(S_Room_List::Room) * roomCount;

			S_ROOMLIST_WRITE pktWriter;
			S_ROOMLIST_WRITE::RoomList roomList = pktWriter.ReserveRoomList(roomCount);

			int idx = 0;
			for (auto& room : rooms) {

				if (room.second->GetIsGameStart() == true)
					continue;

				roomList[idx++] = S_Room_List::Room{ NetRoomInfo{room.second->GetRoomInfo()}};
			}

			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			session->DoSend(sendBuffer);
		}
	}

	//{
	//	// DB에 해당 유저의 ID가 있는지 확인
	//	string inputId = pkt.id; // 서버가 받은 id
	//
	//	try {
	//		std::unique_ptr<sql::PreparedStatement> pstmt(CON->prepareStatement(
	//			"SELECT id, password, name FROM users WHERE id = ?"
	//		));
	//		pstmt->setString(1, to_utf8(inputId));
	//
	//		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
	//
	//		if (res->next()) {
	//			// row가 existence → 해당 id가 DB에 존재함
	//			string id = res->getString("id");
	//			std::string pw = res->getString("password");
	//			std::string name = res->getString("name");
	//
	//			// 비밀번호 검사
	//			if (to_utf8(pkt.password) == pw) {
	//
	//				// CUser 생성 (생성자 안에서 ID발급)
	//				// DB랑 관련 없는 ID
	//				// 서버가 map 자료구조에서 관리하기
	//				// 위해 따로 발급 하는 ID이다. 
	//				shared_ptr<CUser> user = make_shared<CUser>();
	//
	//				// 유저 이름 부여
	//				user->SetName(to_ansi(name));
	//
	//				// 세션이 User 참조 RefCount 증가
	//				CAST_CS(session)->SetUser(user);
	//
	//				// 유저도 자신의 세션을 들고 있는다. (weak 참조)
	//				user->SetSession(session);
	//
	//				// CUser 컨테이너에 저장
	//				EnterUser(user);
	//
	//				// 로그인 허락 패킷 생성
	//				S_LOGIN loginPkt;
	//				loginPkt.success = true;
	//				loginPkt.user_id = user->GetUserID();
	//				COPY_STRING(loginPkt.name, to_ansi(name).c_str());
	//
	//				SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(loginPkt);
	//
	//				// 현재 유저에게 입장 허락 패킷을 보낸다
	//				session->DoSend(sendBuffer);
	//
	//			}
	//			else {
	//				// 유저가 잘못된 비밀번호를
	//				// 써서 로그인 실패
	//				S_LOGIN failPkt;
	//				failPkt.success = false;
	//				SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
	//				session->DoSend(sendBuffer);
	//			}
	//		}
	//		else {
	//			// next()가 false → row 없음 → 해당 id 존재 X
	//			// client 한테 해당 ID를 가진 유저는 없다고 알린다.
	//			// S_LOGIN_FAIL 패킷을 보낸다. 
	//
	//			S_LOGIN failPkt;
	//			failPkt.success = false;
	//			SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(failPkt);
	//			session->DoSend(sendBuffer);
	//		}
	//	}
	//	catch (sql::SQLException& e) {
	//
	//		cout << "[DB] SQL 예외 발생\n";
	//		cout << "  Error: " << e.what() << "\n";
	//		cout << "  Code:  " << e.getErrorCode() << "\n";
	//		cout << "  State: " << e.getSQLStateCStr() << "\n";
	//
	//		return;  // DB 에러 처리
	//	}
	//}
}

void CTitleScene::HandleLogOut(shared_ptr<Session> session, const C_LOGOUT& pkt)
{
	LeaveUser(pkt.user_id);

	S_LOGOUT logOutPkt;
	logOutPkt.success = true;
	auto sendBuffer = CClientPacketHandler::MakeSendBuffer<S_LOGOUT>(logOutPkt);
	session->DoSend(sendBuffer);
}
