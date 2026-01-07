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
	//bool				Connect();

	int32				OnRecv(BYTE* buffer, int32 len);
	void				Disconnect(const std::string& cause);
	void				Close();
	void				ClearSendBufferQueue();
	
public:
	/* 정보 관련 */
	void				SetNetAddress(NetAddress address) { mNetAddress = address; }
	NetAddress			GetAddress() { return mNetAddress; }

	void				SetPlayerId(int id) { mPlayerId = id; }
	int					GetPlayerId() { return mPlayerId; }

	void				SetSocket(SOCKET socket) { mSocket = socket; }
	SOCKET&				GetSocket() { return mSocket; }
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

	SOCKET			mSocket;
	NetAddress		mNetAddress;
	Atomic<bool>	mConnected = true;
	
	
	// send용
	std::queue<SendBuffer*>	mSendBufferQueue;			// 송신 버퍼 (Queue) 전체 버퍼

	// recv용
	RecvBuffer		mRecvBuffer;			// 수신 버퍼 (Ring) 전체 버퍼
	ProcessPacket	mInputQueue;			// 입력 큐 로직으로 입력 전송


	uint32_t		mLastRecvServerTick;
	int				mPlayerId;

	InputCommand	mTempInputCommand;
	SendRequest		mTempSendRequest;

	friend class NetworkThread;
	friend class SessionManager;
};
