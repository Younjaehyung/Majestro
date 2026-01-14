#include "pch.h"
#include "NetRecvSystem.h"
#include "World.h"
#include "ServerCore.h"
#include "NetEntityComponent.h"
#include "InputComponent.h"

NetRecvSystem::NetRecvSystem(World* world) : System(world)
{
}

void NetRecvSystem::Update(float dt)
{
	constexpr int kMaxMsgsPerTick = 256;
	int processed = 0;
	while (processed < kMaxMsgsPerTick && gRecvQueue.Pop(mInputCommand)) {

		switch (mInputCommand.Type)
		{
			case PKT_Type::C2S_PKT_INPUT:
			{
				const InputFrame* inputFrame = mInputCommand.ViewAs<InputFrame>();
				if (inputFrame)
				{
					RecvInput(mInputCommand.SessionId, *inputFrame);
				}
				break;
			}
		}
		++processed;
		
	}
}

void NetRecvSystem::RecvInput(uint32 sessionId, const InputFrame& inputFrame)
{
	auto view = mWorld->GetEntitiesWithComponent<InputComponent>();
	for (auto entity : view)
	{
		InputComponent* inputComp = mWorld->GetComponent<InputComponent>(entity);
		NetEntityComponent* netComp = mWorld->GetComponent<NetEntityComponent>(entity);
		if (netComp && netComp->mSessionId == sessionId)
		{
			// 중복/역순 입력 방지
			if (inputFrame.Seq <= inputComp->lastSeq)
				return;
			inputComp->MoveX = inputFrame.MoveX;
			inputComp->MoveY = inputFrame.MoveY;
			inputComp->Buttons = inputFrame.Buttons;
			inputComp->Yaw = inputFrame.Yaw;
			inputComp->Pitch = inputFrame.Pitch;
			inputComp->lastSeq = inputFrame.Seq;
			break;
		}
	}
}

