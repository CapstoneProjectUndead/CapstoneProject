#include "pch.h"
// 서버쪽 TitleScene
#include "TitleScene.h"
#include "ClientSession.h"
#include "Player.h"

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

void CTitleScene::HandleSignUp(shared_ptr<Session> session, const C_SIGNUP& pkt)
{
	string id = to_utf8(pkt.id);
	string pw = to_utf8(pkt.password);
	string username = to_utf8(pkt.name);

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
	// CUser 생성
	// ID 발급
	// 클라한테 ID와 함께 로그인 허락 패킷을 보낸다. (S_LOGIN)

	// 싱글
	if (!pkt.is_multi) {
		S_LOGIN loginPkt;
		loginPkt.success = true;
		loginPkt.is_multi = false;
		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(loginPkt);
		session->DoSend(sendBuffer);
	}
	// 멀티
	else {
		S_LOGIN loginPkt;
		loginPkt.success = true;
		loginPkt.is_multi = true;
		SendBufferRef sendBuffer = CClientPacketHandler::MakeSendBuffer(loginPkt);
		session->DoSend(sendBuffer);
	}
}
