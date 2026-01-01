#include "pch.h"
#include "CoreGlobal.h"
#include "SocketUtils.h"
#include "ThreadManager.h"
#include "NetworkThread.h"

SessionManager::SessionManager()
{

}

SessionManager::~SessionManager()
{
	
}

void SessionManager::Initialize()
{
	mSessions.clear();
	mPlayerLastIndex = 0;

}



std::shared_ptr<Session>	 SessionManager::CreateSessions(SOCKET sock)
{
	std::shared_ptr<Session> session = std::make_shared<Session>();
	session->SetSession(sock);

	return session;
}

void SessionManager::AddSession(std::shared_ptr<Session>& session)
{
	session->SetPlayerId(++mPlayerLastIndex);
	mSessions.insert(session);
}

void SessionManager::RemoveSessionAt(int32 index)
{
	mSessions.erase(std::next(mSessions.begin(), index));
}

void SessionManager::RemoveSession(std::shared_ptr<Session> session)
{
	auto it = std::find(mSessions.begin(), mSessions.end(), session);
	if (it != mSessions.end())
	{
		mSessions.erase(it);
	}
}

void SessionManager::ClearSessions()
{
	mSessions.clear();
}

void SessionManager::Broadcast(SendBufferRef sendBuffer)
{
	for (auto& session : mSessions)
	{
		session->Send(sendBuffer);
	}
}

