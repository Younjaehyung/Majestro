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
	mSocket = INVALID_SOCKET;
}

Session::~Session()
{
	Close();
	ClearSendBufferQueue();
}

void Session::SetSession(SOCKET socket)
{
	mSocket = socket;
	// 데이터 등록
	SocketUtils::GetNetAddress(socket, mNetAddress);

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
	SocketUtils::Close(mSocket);
}

void Session::ClearSendBufferQueue()
{
	while (!mSendBufferQueue.empty())
	{
		SendBuffer* buffer = mSendBufferQueue.front();
		mSendBufferQueue.pop();
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

void Session::Process(BYTE* buffer, int32 len)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	BYTE* payload = buffer + sizeof(PacketHeader);
	int32 payloadSize = header.Size - sizeof(PacketHeader);

	mTempInputCommand.SessionId = mPlayerId;


	switch (header.PacketType)
	{
	case PKT_Type::KSYNC:
		//mProcessPacket.ProcessSyncPacket(buffer, len);
		break;
	case PKT_Type::KINPUT:
		//mProcessPacket.ProcessInputPacket(buffer, len);
		break;
	case PKT_Type::KACTION:
		//mProcessPacket.ProcessActionPacket(buffer, len);
		break;
	case PKT_Type::KPOSITION:
		//mProcessPacket.ProcessPositionPacket(buffer, len);
		break;
	case PKT_Type::KMSG:
		//mProcessPacket.ProcessMsgPacket(buffer, len);
		break;
	default:
		LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		break;
	}



	gRecvQueue.Push(mTempInputCommand);
}

void Session::SendData(SendBuffer* sendBuffer)
{
	mSendBufferQueue.push(sendBuffer);
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
		Process(buffer + processLen, header.Size);


		processLen += header.Size;
	}


	return processLen;
}

void Session::OnSend(PacketHeader* sendData)
{

	SendBuffer* sendBuffer = SendBufferManager::Acquire();
	sendBuffer->SetData(sendData, sendData->Size);
	mSendBufferQueue.push(sendBuffer);
}