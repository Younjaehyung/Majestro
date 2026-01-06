#include "pch.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "NetworkThread.h"
#include "SocketUtils.h"
#include "PacketHelper.h"

SpscRingQueue<SendRequest, 128*1024>								gSendQueue;
SpscRingQueue<InputCommand, 128*1024>								gRecvQueue;


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
	SendBufferManager::Initialize(1000);
	mNetworkThread = make_shared<NetworkThread>();
}

void ServerCore::Start()
{
	
	mNetworkThread->Start();
}

void ServerCore::Update()
{
	SendRequest data;

	data.SessionId = 1; // Broadcast
	data.Type = PKT_Type::KSYNC;
	data.sync.clientId = 1;
	data.sync.rhythmTime = 123.456f;

	Sleep(1); // Simulate some processing delay


	UnicastPacket(data);
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
