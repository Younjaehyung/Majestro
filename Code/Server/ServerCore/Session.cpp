#include "pch.h"
#include "Session.h"
#include <atomic>
#include "ServerCore.h"
#include "SocketUtils.h"
#include "SessionManager.h"
#include "SendBuffer.h"
#include "Packet.h"

/*--------------
	Session
---------------*/

Session::Session() : mRecvBuffer(BUFFER_SIZE)
{
	mTcpSocket = INVALID_SOCKET;
	
}

Session::~Session()
{
	Close();
	ClearSendBufferQueue();
}

void Session::SetSession(SOCKET& tcpsocket, SOCKET& udpsocket)
{
	mTcpSocket = tcpsocket;
	
	// 데이터 등록
	SocketUtils::GetNetAddress(mTcpSocket, mTcpAddr);
	SocketUtils::GetNetAddress(udpsocket, mUdpAddr);

}


////bool Session::Connect()
////{
////	//return RegisterConnect();
////}



void Session::Disconnect(const std::string& cause)
{
	if (mConnected.exchange(false) == false)
		return;


	LOG_INFO("Disconnect Req ID :[{}] Cause:{} ",
		mPlayerId, cause);
}

void Session::Close()
{
	LOG_INFO("Disconnect ID :[{}] ",
		mPlayerId);
	SocketUtils::Close(mTcpSocket);
	
}

void Session::ClearSendBufferQueue()
{
	while (!mTSendBufferQueue.empty())
	{
		SendBuffer* buffer = mTSendBufferQueue.front();
		mTSendBufferQueue.pop();
		SendBufferManager::Release(buffer);
	}
}


void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect("HandleError");
		break;
	default:
		// TODO : Log
		LOG_ERROR("Handle Error Code [{}]", errorCode);
		break;
	}
}

void Session::SendTcpData(SendBuffer* sendBuffer)
{
	mTSendBufferQueue.push(sendBuffer);
}

void Session::SendUdpData(SendBuffer* sendBuffer)
{
	mUSendBufferQueue.push(sendBuffer);
}

int32 Session::OnTcpRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketTcpHeader))
			break;

		PacketTcpHeader header;
		::memcpy(&header, buffer + processLen, sizeof(PacketTcpHeader));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다

		if (header.Header.Size < sizeof(PacketTcpHeader))
			return -1; // 프로토콜 오류

		if (dataSize < header.Header.Size)
			break; // 아직 덜 옴

		// 패킷 조립 성공
		mTempInputCommand.SessionId = mPlayerId;
		ProcessPacket::ProcessPackets(mTempInputCommand, buffer);
		gRecvQueue.Push(mTempInputCommand);

		processLen += header.Header.Size;
	}


	return processLen;
}

int32 Session::OnUdpRecv(BYTE* buffer, int32 len)
{
	mTempInputCommand.SessionId = mPlayerId;
	ProcessPacket::ProcessPackets(mTempInputCommand, buffer);
	gRecvQueue.Push(mTempInputCommand);

	return len;
}
