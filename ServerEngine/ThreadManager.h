#pragma once

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void Launch(std::function<void(void)> callback);
	void Join();

	static void InitTLS();
	static void DestroyTLS();

private:
	std::mutex					lock;
	std::vector<std::thread>	threads;
};

