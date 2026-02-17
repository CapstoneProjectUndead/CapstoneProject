#pragma once

class ThreadManager
{
public:
	ThreadManager();
	~ThreadManager();

	void	Launch(std::function<void(void)> callback);
	void	Join();

	//		스레드 풀 초기화 (로직용 스레드들을 미리 생성)
	void	InitThreadPool(int threadCount);

	//		일감(Task)을 던지는 함수
	void	PushTask(std::function<void()> task);

	static void InitTLS();
	static void DestroyTLS();

private:
	//      스레드들이 수행할 실제 루프 함수
	void	DoWorkerJob();

private:
	std::mutex					 lock;
	std::vector<std::thread>	 threads;

	// 일감 큐와 동기화 도구
	std::mutex					 job_queue_lock;
	std::condition_variable		 cv;
	std::queue<std::function<void()>> job_queue;
	bool						 shut_down = false; // 스레드 종료 플래그
};

