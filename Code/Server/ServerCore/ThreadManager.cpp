#include "pch.h"
#include "ThreadManager.h"

#include "CoreTLS.h"
#include "CoreGlobal.h"


ThreadManager::ThreadManager()
{
	InitTLS();
}

ThreadManager::~ThreadManager()
{
	Join();
}

void ThreadManager::Launch(function<void()> callback)
{
	LockGuard guard(_lock);
	_threads.emplace_back(thread([=]() {
		InitTLS();
		callback();
		DestroyTLS();
		}));
}

void ThreadManager::Join()
{
	for (thread& t: _threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}
	_threads.clear();
}

void ThreadManager::InitTLS()
{
	static Atomic<uint32> s_nextThreadID = 1;
	LThreadID = s_nextThreadID.fetch_add(1);
}

void ThreadManager::DestroyTLS()
{

}
