#pragma once
#include "NetAddress.h"
#include <map>
#include "Session.h"
#include "SendBuffer.h"

class ServerCore;
class Session;
class NetworkThread;

// sockaddr_in을 key로 쓰기 위한 비교 연산자 정의
struct sockaddr_compare {
    bool operator()(const sockaddr_in& lhs, const sockaddr_in& rhs) const {
        if (lhs.sin_addr.s_addr != rhs.sin_addr.s_addr)
            return lhs.sin_addr.s_addr < rhs.sin_addr.s_addr;
        return lhs.sin_port < rhs.sin_port;
    }
};


class SessionManager
{
public:
	std::map<uint64, std::shared_ptr<Session>>		mSessions;
	std::map<sockaddr_in, uint64, sockaddr_compare> mUdpMapper;
public:
	SessionManager();
	~SessionManager();

	void 								Initialize();

	std::shared_ptr<Session>			CreateSessions(SOCKET& tcpsock, SOCKET& udpsock);
	void								RemoveSessionAt(int32 index);
	void								RemoveSession(std::shared_ptr<Session> session);
	void 								ClearSessions();
	shared_ptr<Session> FindSessionByAddr(sockaddr_in addr);
	void RegisterUdpAddress(sockaddr_in addr, uint64 sessionId);
public:
	std::map<uint64, std::shared_ptr<Session>>&		GetAllSessions() { return mSessions; }

	size_t									GetMaxSessionCount() { return mSessions.size(); }
	void 									Update() {};
private:
	uint8										mCoreState = 0; // 0: Init, 1: Running, 2: Stop		
	uint64										mPlayerLastIndex = 0;


};
