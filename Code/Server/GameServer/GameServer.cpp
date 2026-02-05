#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include "Timer.h"
#include "ServerCore.h"
#include "GameCore.h"

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
		gGameCore->Update(gTimer.GetTimeElapsed()); // Assuming a fixed delta time for simplicity
    }

}
