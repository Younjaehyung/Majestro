#include "pch.h"
#include "Session.h"
#include <atomic>
#include "SocketUtils.h"
#include "Packet.h"
/*--------------
	Session
---------------*/

Session::Session() : mRecvBuffer(BUFFER_SIZE)
{
	mSocket = INVALID_SOCKET;
}

Session::~Session()
{
	SocketUtils::Close(mSocket);
}

void Session::SetSession(SOCKET socket)
{
	mSocket = socket;
	// 데이터 등록
	SocketUtils::GetNetAddress(socket, mNetAddress);

}
//
//void Session::Send(SendBufferRef sendBuffer)
//{
//	if (IsConnected() == false)
//		return;
//
//	bool registerSend = false;
//
//	//// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어준다
//	//{
//	//	//WRITE_LOCK;
//
//	//	_sendQueue.push(sendBuffer);
//
//	//	if (_sendRegistered.exchange(true) == false)
//	//		registerSend = true;
//	//}
//
//	//if (registerSend)
//	//	RegisterSend();
//}

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
	SocketUtils::Close(mSocket);
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

int32 Session::OnRecv(BYTE* buffer, int32 len)
{
	int32 processLen = 0;

	while (true)
	{
		int32 dataSize = len - processLen;
		// 최소한 헤더는 파싱할 수 있어야 한다
		if (dataSize < sizeof(PacketHeader))
			break;

		PacketHeader header;
		::memcpy(&header, buffer + processLen, sizeof(PacketHeader));
		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다

		if (header.Size < sizeof(PacketHeader))
			return -1; // 프로토콜 오류

		if (dataSize < header.Size)
			break; // 아직 덜 옴

		// 패킷 조립 성공
		mInputQueue.Process(buffer + processLen, header.Size);

		processLen += header.Size;
	}

	return processLen;
}


void Session::OnSend(int32 len)
{
	if (IsConnected() == false)
		return;
		
	bool registerSend = false;
		
	{
		_sendQueue.push(sendBuffer);
		
		if (_sendRegistered.exchange(true) == false)
			registerSend = true;
	}
		
	if (registerSend)
		RegisterSend();
}
