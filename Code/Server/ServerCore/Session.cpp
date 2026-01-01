#include "pch.h"
#include "Session.h"
#include <atomic>
#include "SocketUtils.h"

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

void Session::Send(SendBufferRef sendBuffer)
{
	if (IsConnected() == false)
		return;

	bool registerSend = false;

	//// 현재 RegisterSend가 걸리지 않은 상태라면, 걸어준다
	//{
	//	//WRITE_LOCK;

	//	_sendQueue.push(sendBuffer);

	//	if (_sendRegistered.exchange(true) == false)
	//		registerSend = true;
	//}

	//if (registerSend)
	//	RegisterSend();
}

//bool Session::Connect()
//{
//	//return RegisterConnect();
//}

void Session::Disconnect(const WCHAR* cause)
{
	if (mConnected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect : " << cause << endl;

//	RegisterDisconnect();
}



void Session::HandleError(int32 errorCode)
{
	switch (errorCode)
	{
	case WSAECONNRESET:
	case WSAECONNABORTED:
		Disconnect(L"HandleError");
		break;
	default:
		// TODO : Log
		cout << "Handle Error : " << errorCode << endl;
		break;
	}
}

/*-----------------
	PacketSession
------------------*/
//
//PacketSession::PacketSession()
//{
//}
//
//PacketSession::~PacketSession()
//{
//}
//
//// [size(2)][id(2)][data....][size(2)][id(2)][data....]
//int32 PacketSession::OnRecv(BYTE* buffer, int32 len)
//{
//	int32 processLen = 0;
//
//	while (true)
//	{
//		int32 dataSize = len - processLen;
//		// 최소한 헤더는 파싱할 수 있어야 한다
//		if (dataSize < sizeof(PacketHeader))
//			break;
//
//		PacketHeader header = *(reinterpret_cast<PacketHeader*>(&buffer[processLen]));
//		// 헤더에 기록된 패킷 크기를 파싱할 수 있어야 한다
//		if (dataSize < header.size)
//			break;
//
//		// 패킷 조립 성공
//		OnRecvPacket(&buffer[processLen], header.size);
//
//		processLen += header.size;
//	}
//
//	return processLen;
//}
