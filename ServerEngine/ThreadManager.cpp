#include "pch.h"
#include "ThreadManager.h"
#include "TLS.h"

ThreadManager::ThreadManager()
{
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void(void)> callback)
{
	lock_guard<mutex> lg(lock);

	threads.push_back(thread([=]()
		{
			InitTLS();
			callback();
			DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (thread& t : threads)
	{
		if (t.joinable())
			t.join();
	}
}

void ThreadManager::InitTLS()
{
	static atomic<uint32> SThreadID = 1;
	LThreadID = SThreadID.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{
	// 위 InitTLS 함수에서 동적할당 한 메모리가 있으면
	// 여기서 반환해 준다.
}
