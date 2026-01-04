#include "SessionManager.h"
#include "pch.h"
#include "SessionManager.h"
#include "SocketUtils.h"
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
	session->GetConnectedAtomic() = true;
	mSessions[mPlayerLastIndex]=session;
}

void SessionManager::RemoveSessionAt(int32 index)
{
	mSessions.erase(index);
}

void SessionManager::RemoveSession(std::shared_ptr<Session> session)
{
	mSessions.erase(session->GetPlayerId());
}

void SessionManager::ClearSessions()
{
	mSessions.clear();
}


