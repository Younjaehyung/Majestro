#include "pch.h"
#include "ServerCore.h"
#include "ThreadManager.h"
#include "SessionManager.h"
#include "NetworkThread.h"
#include "SocketUtils.h"

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
	// 扩加 檬扁拳
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	// 家南 积己
	mListenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSocket == INVALID_SOCKET) LOG_ERROR("err(socket)");

	// bind()
	if (false ==SocketUtils::BindAnyAddress(mListenSocket, 9000)) {
		LOG_ERROR("err(bind)");
		SocketUtils::Close(mListenSocket);
		SocketUtils::Clear();
		return;
	}
	
	// listen()
	if (false == SocketUtils::Listen(mListenSocket, SOMAXCONN)) {
		LOG_ERROR("err(listen)");
		SocketUtils::Close(mListenSocket);
		SocketUtils::Clear();
		return;
	}

	LOG_INFO("START GAME SERVER");


	gSessionMgr.Initialize();
	mNetworkThread = make_shared<NetworkThread>(mListenSocket);
}

void ServerCore::Start()
{

	mNetworkThread->Start();
}

void ServerCore::Update()
{
	gSessionMgr.Update();

}

void ServerCore::Stop()
{
	mNetworkThread->Stop();
	gSessionMgr.ClearSessions();
}