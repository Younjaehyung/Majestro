#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include "GameTimer.h"
#include "ServerCore.h"
#include "GameCore.h"
#include "McpBridge.h"

Timer                   gTimer;

int main()
{

    gTimer.Start();
    gServerCore->Initialize();
	gGameCore->Initialize();


    gServerCore->Start();
    gGameCore->Start();

    while (true)
    {
        gTimer.Tick(60.f);
        //gServerCore.Update();
       // Sleep(30); // Simulate some processing delay
		// -mcp: MCP 상태 조회를 로직 스레드에서 틱 시작에 처리한다 (World 를 잠금 없이 읽는다).
		if (McpBridge::Requested())
			McpBridge::Get().Poll();
		gGameCore->Update(gTimer.GetTimeElapsed()); // Assuming a fixed delta time for simplicity
    }

}
