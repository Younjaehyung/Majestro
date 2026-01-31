#include "pch.h"
#include "ServerCore.h"
#include "SessionManager.h"
#include "NetworkThread.h"
#include "SocketUtils.h"

SpscRingQueue<SendRequest, 128*1024>								gSendQueue;
SpscRingQueue<InputCommand, 128*1024>								gRecvQueue;
SpscRingQueue<uint32, 1024>											gNewSessions;


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

	data.SessionId = 0; // Broadcast
	data.Type = PKT_Type::PKT_TCP;
	

	//data.sync.rhythmTime = 123.456f;

	//

	//for(int i =1; i < 5; ++i)
	//{
	//	//data.SessionId = i;
	//	//
	//	//data.tcpHeader = PacketTcpHeader{ sizeof(PacketTcpHeader), PKT_Type::PKT_TCP, 0.0 };
	//	//data.Type = PKT_Type::PKT_TCP;
	//	//UnicastPacket(data);
	//	//
	//	//data.udpHeader = PacketUdpHeader{ sizeof(PacketUdpHeader), PKT_Type::PKT_UDP, (uint32)i, 0 };
	//	//data.Type = PKT_Type::PKT_UDP;
	//	//UnicastPacket(data);


	//}
	//
	
}

void ServerCore::Stop()
{
	mNetworkThread->Stop();
}

void ServerCore::BroadcastPacket(SendRequest& pkt)
{

}

void ServerCore::UnicastPacket(SendRequest& pkt)
{
	gSendQueue.Push(pkt);
}
