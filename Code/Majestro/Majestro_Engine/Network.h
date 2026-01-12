#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include "RingBuffer.h"
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "SpscRingQueue.h"


constexpr const char* SERVERIP = "127.0.0.1";
constexpr int TCPSERVERPORT = 9000;	//TCP
constexpr int UDPSERVERPORT = 9001;	//UDP
constexpr int BUFSIZE = 4096;

extern SpscRingQueue<SendRequest, 128>	gSendBuffer;	// Logic -> Network
extern SpscRingQueue<InputCommand, 128>	gRecvBuffer;	// Network -> Logic

class Network
{
public:
	uint32  mClientId = 0;
private:
	WSADATA mWsaData{};
	SOCKET	mTcpSocket;
	SOCKET	mUdpSocket;
	sockaddr_in mServerTcpAddr{};
	sockaddr_in mServerUdpAddr{};
	
private:
	std::thread				mNetworkThread;
	std::atomic<bool>		mIsRunning;

	BYTE					mURecvBuffer[BUFSIZE];
	RecvBuffer				mTRecvBuffer;	// 수신 버퍼
	std::queue<SendBuffer*> mSendBuffer;	// 전송 버퍼 큐
private:
	InputCommand	mInputCommand;
	SendRequest		mSendData;
private:

	Network();
	~Network();
public: // Init
	static Network& GetInstance() {
		static Network instance;
		return instance;
	}

public: // 외부통신용
	void Initialize();
	
	void Awake();
	void Stop();
	void Shutdown();

private: // Session

	// send
	void PrepareSendData();
	void OnSendPacket();

	// recv
	void  OnRecvPacket();	// recv process
	void  OnTCPNetworkUpdate();
	int32 OnTcpRecv(BYTE* buffer, int32 len);

	void OnUDPNetworkUpdate();
	

private: // Process
	bool ConnectToServer(const char* ipAddress = SERVERIP, int port = TCPSERVERPORT);
	void ReleaseServer();
	void NetworkUpdate();
private:
	
	
};

