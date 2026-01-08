#include "pch.h"
#include "Network.h"
#include "PacketHelper.h"
#include "Packet.h"

int main()
{
	Network::GetInstance().Initialize();
	Network::GetInstance().Awake();


	while(true)
	{
		//Network::GetInstance().GameRecvUpdate();
		gSendBuffer.Push({
			PKT_Type::KTCP,
			PacketTcpHeader{ sizeof(PacketTcpHeader), PKT_Type::KTCP, 0.0 }
			});
		gSendBuffer.Push({
			PKT_Type::KUDP,
			PacketUdpHeader{ sizeof(PacketUdpHeader), PKT_Type::KUDP, 1, 0 }
			});
		Sleep(30); // Simulate some processing delay
		//Network::GetInstance().GameSendUpdate();
	}
}