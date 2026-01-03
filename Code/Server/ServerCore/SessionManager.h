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
	SessionManager();
	~SessionManager();

	void 								Initialize();

	std::shared_ptr<Session>			CreateSessions(SOCKET);
	void								AddSession(std::shared_ptr<Session>& seesion);
	void								RemoveSessionAt(int32 index);
	void								RemoveSession(std::shared_ptr<Session> session);
	void 								ClearSessions();

	InputCommand*						PopData(uint32 sId);
	
	void 								Broadcast(PacketHeader* sendBuffer);
	void								Unicast(int32 playerId, PacketHeader* sendBuffer);
public:
	std::map<uint8, std::shared_ptr<Session>>&		GetAllSessions() { return mSessions; }

	size_t									GetMaxSessionCount() { return mSessions.size(); }
	void 									Update() {};
private:
	uint8										mCoreState = 0; // 0: Init, 1: Running, 2: Stop		
	uint8										mPlayerLastIndex = 0;
private:

	std::map<uint8, std::shared_ptr<Session>>		mSessions;

};
