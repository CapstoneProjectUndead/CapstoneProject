#pragma once

class CServerSession;

class CSessionManager
{
private:
	CSessionManager();
	CSessionManager(const CSessionManager&) = delete;

public:
	~CSessionManager();

	static CSessionManager& GetInstance() {
		static CSessionManager instance;
		return instance;
	}

public:
	void SetServerSession(std::shared_ptr<CServerSession> session) { server_session = session; };
	std::shared_ptr<CServerSession> GetServerSession() const { return server_session; }

private:
	std::shared_ptr<CServerSession> server_session;
};

