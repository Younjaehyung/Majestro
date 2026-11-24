#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include "GameTimer.h"
#include "ServerCore.h"
#include "GameCore.h"
#include "McpBridge.h"
#include "CrashHandler.h"

Timer                   gTimer;

int main()
{
    // 처리되지 않은 예외로 죽으면 작업 폴더 Dumps/ 에 미니덤프와 호출 스택을 남긴다.
    CrashHandler::Install(L"GameServer");

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
