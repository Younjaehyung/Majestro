#pragma once
#include "NetAddress.h"
#include <string>
#include <queue>
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "PacketHelper.h"

/*--------------
	Session
---------------*/

constexpr int BUFSIZE = 4096;

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
	void				SetSession(SOCKET&, SOCKET&);
	//bool				Connect();

	
	int32				OnTcpRecv(BYTE* buffer, int32 len);
	int32				OnUdpRecv(BYTE* buffer, int32 len);
	void				Disconnect(const std::string& cause);
	void				Close();
	void				ClearSendBufferQueue();
	
public:
	/* 정보 관련 */
	void				SetTNetAddress(NetAddress address) { mTcpAddr = address; }
	void				SetUNetAddress(NetAddress address) { mUdpAddr = address; }
	
	NetAddress			GetTcpAddress() { return mTcpAddr; }
	NetAddress			GetUdpAddress() { return mUdpAddr; }

	void				SetPlayerId(int id) { mPlayerId = id; }
	int					GetPlayerId() { return mPlayerId; }

	void				SetTSocket(SOCKET socket) { mTcpSocket = socket; }

	SOCKET&				GetTSocket() { return mTcpSocket; }
	Atomic<bool>&		IsConnected()		{ return mConnected; }
	
private:
	void				HandleError(int32 errorCode);

protected:
	/* 컨텐츠 코드에서 재정의 */
	virtual void		OnConnected() {}
	virtual void		OnDisconnected() {}
private:
	void SendData(SendBuffer* sendBuffer);

private:

	SOCKET			mTcpSocket;

	NetAddress		mTcpAddr;
	NetAddress		mUdpAddr;
	Atomic<bool>	mConnected = true;
	
	
	// send용
	std::queue<SendBuffer*>	mTSendBufferQueue;			// 송신 버퍼 (Queue) 전체 버퍼
	std::queue<SendBuffer*>	mUSendBufferQueue;			// 송신 버퍼 (Queue) 전체 버퍼

	// recv용
	RecvBuffer		mRecvBuffer;			// 수신 버퍼 (Ring) 전체 버퍼
	BYTE			mURecvBuffer[4096];	// UDP 수신 버퍼

	uint32_t		mLastRecvServerTick;
	int				mPlayerId;

	InputCommand	mTempInputCommand;
	SendRequest		mTempSendRequest;

	friend class NetworkThread;
	friend class SessionManager;
};
