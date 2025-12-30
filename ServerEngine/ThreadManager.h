#pragma once

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void Launch(function<void(void)> callback);
	void Join();

	static void InitTLS();
	static void DestroyTLS();

private:
	mutex			lock;
	vector<thread>	threads;
};

