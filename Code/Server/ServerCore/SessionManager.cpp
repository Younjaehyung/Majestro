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

std::shared_ptr<Session>	 SessionManager::CreateSessions(SOCKET& tcpsock, SOCKET& udpsock)
{
	std::shared_ptr<Session> session = std::make_shared<Session>();
	session->SetSession(tcpsock, udpsock);
	session->SetPlayerId(++mPlayerLastIndex);
	session->IsConnected() = true;
	mSessions[mPlayerLastIndex] = session;

	return session;
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

shared_ptr<Session> SessionManager::FindSessionByAddr(sockaddr_in addr)
{
	
	auto it = mUdpMapper.find(addr);
	if (it == mUdpMapper.end()) return nullptr;
	return mSessions[it->second];
	
}

void SessionManager::RegisterUdpAddress(sockaddr_in addr, uint64 sessionId)
{
	
	mUdpMapper[addr] = sessionId;
	LOG_INFO("Session {} UDP Address Registered!", sessionId);
	LOG_INFO("Addr IP: {}, Port: {}", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	
}


