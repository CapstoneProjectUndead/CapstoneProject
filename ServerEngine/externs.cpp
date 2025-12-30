#include "pch.h"
#include "externs.h"
#include "SocketHelper.h"
#include "ThreadManager.h"

ThreadManager* GThreadManager = nullptr;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		SocketHelper::Init();
		GThreadManager = new ThreadManager();
	}

	~CoreGlobal()
	{
		SocketHelper::Clear();
		delete GThreadManager;
	}
}GCoreGlobal;