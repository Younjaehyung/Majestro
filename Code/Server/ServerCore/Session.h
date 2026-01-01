#pragma once
#include "NetAddress.h"
#include <string>
#include "RecvBuffer.h"
#include "SendBuffer.h"

/*--------------
	Session
---------------*/

class Session
{
	enum
	{
		BUFFER_SIZE = 0x10000, // 64KB
	};

public:
	Session();
	virtual ~Session();

public:
	/* 외부에서 사용 */
	void				SetSession(SOCKET socket);
	//void				Send(SendBufferRef sendBuffer);
	//bool				Connect();
	void				Disconnect(const std::string& cause);
	void				Close();
public:
	/* 정보 관련 */
	void				SetNetAddress(NetAddress address) { mNetAddress = address; }
	NetAddress			GetAddress() { return mNetAddress; }

	void				SetPlayerId(int id) { mPlayerId = id; }
	int					GetPlayerId() { return mPlayerId; }

	void				SetSocket(SOCKET socket) { mSocket = socket; }
	SOCKET				GetSocket() { return mSocket; }
	bool				IsConnected() { return mConnected; }
	
	Atomic<bool>&		GetConnectedAtomic() { return mConnected; }

private:

private:
	/* 전송 관련 */
	/*void				ProcessConnect();
	void				ProcessDisconnect();
	void				ProcessRecv(int32 numOfBytes);
	void				ProcessSend(int32 numOfBytes);*/

	void				HandleError(int32 errorCode);

protected:
	/* 컨텐츠 코드에서 재정의 */
	virtual void		OnConnected() {}
	virtual int32		OnRecv(BYTE* buffer, int32 len) { return len; }
	virtual void		OnSend(int32 len) {}
	virtual void		OnDisconnected() {}

private:

	SOCKET			mSocket;
	Atomic<bool>	mConnected = false;

	NetAddress		mNetAddress;
	RecvBuffer		mRecvBuffer;
	//SendBuffer		mSendBuffer;

	uint32_t		mLastRecvServerTick;
	int				mPlayerId;

	friend class NetworkThread;
	friend class SessionManager;
};


/*-----------------
	PacketSession
------------------*/

//struct PacketHeader
//{
//	uint16 size;
//	uint16 id; // 프로토콜ID (ex. 1=로그인, 2=이동요청)
//};
//
//class PacketSession : public Session
//{
//public:
//	PacketSession();
//	virtual ~PacketSession();
//
//	PacketSessionRef	GetPacketSessionRef() { return static_pointer_cast<PacketSession>(shared_from_this()); }
//
//protected:
//	virtual int32		OnRecv(BYTE* buffer, int32 len) sealed;
//	virtual void		OnRecvPacket(BYTE* buffer, int32 len) abstract;
//};