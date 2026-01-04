#include "pch.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "NetworkThread.h"
#include "SocketUtils.h"

SpscRingQueue<SendRequest, 128>								gSendQueue;
SpscRingQueue<InputCommand, 128>						gRecvQueue;


ServerCore::ServerCore()
{


}

ServerCore::~ServerCore()
{
}

void ServerCore::Initialize()
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return;

	// 소켓 생성
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

	mNetworkThread = make_shared<NetworkThread>(mListenSocket);
}

void ServerCore::Start()
{
	
	mNetworkThread->Start();
}

void ServerCore::Update()
{
}

void ServerCore::Stop()
{
	mNetworkThread->Stop();
}

void ServerCore::BroadcastPacket(SendRequest& pkt)
{
	mNetworkThread->BroadcastPacket(pkt);
}

void ServerCore::UnicastPacket(SendRequest& pkt)
{
	gSendQueue.Push(pkt);
}
