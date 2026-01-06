#include "pch.h"
#include "Network.h"

int main()
{
	Network::GetInstance().Initialize();
	Network::GetInstance().ConnectToServer();



	while(true)
	{
		Network::GetInstance().GameRecvUpdate();

		//Network::GetInstance().GameSendUpdate();
	}
}