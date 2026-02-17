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
	// 종료 신호를 보내고 다 깨운다
	{
		lock_guard<mutex> lg(job_queue_lock);
		shut_down = true;
	}
	cv.notify_all();

	for (thread& t : threads)
	{
		if (t.joinable())
			t.join();
	}
	threads.clear();
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

// 로직용 스레드 풀을 생성합니다.
void ThreadManager::InitThreadPool(int threadCount)
{
	for (int i = 0; i < threadCount; i++)
	{
		Launch([this]() { this->DoWorkerJob(); });
	}
}

// 외부에서 일감을 던질 때 사용합니다.
void ThreadManager::PushTask(function<void()> task)
{
	{
		lock_guard<mutex> lg(job_queue_lock);
		job_queue.push(task);
	}

	// 자고 있는 스레드 하나를 깨운다
	cv.notify_one();
}

void ThreadManager::DoWorkerJob()
{
	while (true)
	{
		function<void()> task;

		{
			unique_lock<mutex> lock(job_queue_lock);

			// 일감이 없으면 잘(Sleep) 준비를 한다. 
			// 일감이 들어오거나(_jobQueue) 종료 신호(_shutdown)가 올 때까지 대기.
			cv.wait(lock, [this]() { return !job_queue.empty() || shut_down; });

			if (shut_down && job_queue.empty())
				break;

			// 일감을 꺼낸다
			task = std::move(job_queue.front());
			job_queue.pop();
		}

		// 락을 푼 상태에서 일감을 실행한다 (중요!)
		task();
	}
}
