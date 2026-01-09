#include "pch.h"
#include "Network.h"

int main()
{
	Network::GetInstance().Initialize();
	Network::GetInstance().Awake();


	while(true)
	{
		//Network::GetInstance().GameRecvUpdate();
		gSendBuffer.Push({
			PKT_Type::PKT_TCP,
			PacketTcpHeader{ sizeof(PacketTcpHeader), PKT_Type::PKT_TCP, 0.0 }
			});
		gSendBuffer.Push({
			PKT_Type::PKT_UDP,
			PacketUdpHeader{ sizeof(PacketUdpHeader), PKT_Type::PKT_UDP, 1, 0 }
			});
		Sleep(30); // Simulate some processing delay
		//Network::GetInstance().GameSendUpdate();
	}
}