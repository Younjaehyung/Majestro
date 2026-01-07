#pragma once
#include "NetAddress.h"
#include <map>
#include "Session.h"
#include "SendBuffer.h"

class ServerCore;
class Session;
class NetworkThread;

class SessionManager
{
public:
	std::map<uint32, std::shared_ptr<Session>>		mSessions;
public:
	SessionManager();
	~SessionManager();

	void 								Initialize();

	std::shared_ptr<Session>			CreateSessions(SOCKET&, SOCKET&);
	void								RemoveSessionAt(int32 index);
	void								RemoveSession(std::shared_ptr<Session> session);
	void 								ClearSessions();

public:
	std::map<uint32, std::shared_ptr<Session>>&		GetAllSessions() { return mSessions; }

	size_t									GetMaxSessionCount() { return mSessions.size(); }
	void 									Update() {};
private:
	uint8										mCoreState = 0; // 0: Init, 1: Running, 2: Stop		
	uint32										mPlayerLastIndex = 0;


};
