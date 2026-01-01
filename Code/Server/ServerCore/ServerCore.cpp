#include "pch.h"
#include "ServerCore.h"
#include "ThreadManager.h"
#include "CoreGlobal.h"
#include "NetworkThread.h"

SessionManager gSessionMgr;
ThreadManager* GThreadManager = nullptr;


ServerCore::ServerCore()
{
	GThreadManager = new ThreadManager();


}

ServerCore::~ServerCore()
{
	delete GThreadManager;
}

void ServerCore::Initialize()
{
	gSessionMgr.Initialize();
	mNetworkThread = make_shared<NetworkThread>();
	

}

void ServerCore::Start()
{

	mNetworkThread->Start();
}

void ServerCore::Update()
{
	gSessionMgr.Update();

}