#pragma once
#include "NetAddress.h"

class IOCP
{
public:
	IOCP();
	~IOCP();

	bool Initialize();
	bool Register(HANDLE socket, ULONG_PTR key);
	void WorkerThreadLoop(uint32 timeoutMs = INFINITE);

private:
	HANDLE		iocp_handle;
};

