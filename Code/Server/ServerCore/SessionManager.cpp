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
	auto it = mUdpMapper.find(addr);
	if (it != mUdpMapper.end() && it->second == sessionId)
		return;   // 이미 같은 매핑 — 매 패킷 반복되는 경로

	mUdpMapper[addr] = sessionId;

	// 스레드 안전을 위해 inet_ntop 사용
	char ip[INET_ADDRSTRLEN] = {};
	::inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

	MJLOG_INFO(Session, "UDP 주소 등록 session={} addr={}:{}",
		sessionId, ip, ntohs(addr.sin_port));
}


