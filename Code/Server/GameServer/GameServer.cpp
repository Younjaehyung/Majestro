#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include "Timer.h"
#include "ServerCore.h"
#include "GameCore.h"


void ThreadFunc()
{
    while (true)
    {
        std::cout << "Thread ID: " << LThreadID << " is running." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
	}

}

GameCore                gGameCore;
ServerCore			    gServerCore;
Timer       gTimer;

int main()
{

 //   gTimer.Start();
 //   gServerCore.Initialize();
	//gGameCore.Initialize();


 //   gServerCore.Start();
	//gServerCore.Start();

 //   while (true)
 //   {
 //       gTimer.Tick();
 //       gServerCore.Update();
	//	gGameCore.Update(gTimer.GetTimeElapsed()); // Assuming a fixed delta time for simplicity
 //   }

}
