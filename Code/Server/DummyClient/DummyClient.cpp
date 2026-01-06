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
			PKT_Type::KSYNC,
			SyncPacketData{ 1, 123.456f }
			});
		Sleep(1);
		//Network::GetInstance().GameSendUpdate();
	}
}