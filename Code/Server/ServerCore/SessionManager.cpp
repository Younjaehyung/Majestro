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

InputCommand* SessionManager::PopData(uint32 sId)
{
	if(mSessions[sId]){
		std::lock_guard<std::mutex> lock(mSessions[sId]->mMutex);
		return mSessions[sId]->mInputQueue.PopCommand();
	}
}

void SessionManager::Broadcast(PacketHeader* sendBuffer)
{
	for (auto& session : mSessions)
	{
		std::lock_guard<std::mutex> lock(session.second->mMutex);
		session.second->OnSend(sendBuffer);
	}
}

void SessionManager::Unicast(int32 playerId, PacketHeader* sendBuffer)
{
	if (mSessions[playerId])
	{
		std::lock_guard<std::mutex> lock(mSessions[playerId]->mMutex);
		mSessions[playerId]->OnSend(sendBuffer);
	}
	
}

