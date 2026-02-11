#pragma once

class CServerSession;

class CServerSessionManager
{
private:
	CServerSessionManager();
	CServerSessionManager(const CServerSessionManager&) = delete;

public:
	~CServerSessionManager();

	static CServerSessionManager& GetInstance() {
		static CServerSessionManager instance;
		return instance;
	}

public:
	void SetServerSession(std::shared_ptr<CServerSession> session) { server_session = session; };
	std::shared_ptr<CServerSession> GetServerSession() const { return server_session; }

private:
	std::shared_ptr<CServerSession> server_session;
};

