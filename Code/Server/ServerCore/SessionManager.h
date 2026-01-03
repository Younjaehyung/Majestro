#pragma once
#include "NetAddress.h"
#include <set>
#include "Session.h"
#include "SendBuffer.h"

class ServerCore;
class Session;
class NetworkThread;

class SessionManager
{
public:
	SessionManager();
	~SessionManager();

	void 								Initialize();

	std::shared_ptr<Session>			CreateSessions(SOCKET);
	void								AddSession(std::shared_ptr<Session>& seesion);
	void								RemoveSessionAt(int32 index);
	void								RemoveSession(std::shared_ptr<Session> session);
	void 								ClearSessions();
	
	void 								Broadcast(PacketHeader* sendBuffer);

public:
	std::set<std::shared_ptr<Session>>&		GetAllSessions() { return mSessions; }

	size_t									GetMaxSessionCount() { return mSessions.size(); }

	void 									SetServerAddress(const NetAddress& address) { mServerAddress = address; }
	NetAddress&								GetServerAddress() { return mServerAddress; }

	void									SetServerCore(std::weak_ptr<ServerCore> serverCore) { mServerCore = serverCore; }
	std::weak_ptr<ServerCore>				GetServerCore() { return mServerCore; }

	void 									Update() {};
private:
	uint8										mCoreState = 0; // 0: Init, 1: Running, 2: Stop		
	uint8										mPlayerLastIndex = 0;
private:
	std::weak_ptr<ServerCore>					mServerCore;
	std::set<std::shared_ptr<Session>>			mSessions;

	SOCKET										mSocket;
	NetAddress									mServerAddress = {};
};
