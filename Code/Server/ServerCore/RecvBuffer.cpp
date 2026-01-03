#include "pch.h"
#include "RecvBuffer.h"
#include "PacketHelper.h"
/*--------------
	RecvBuffer
----------------*/

RecvBuffer::RecvBuffer(int32 bufferSize) : mBufferSize(bufferSize)
{
	mCapacity = bufferSize * BUFFER_COUNT;
	mBuffer.resize(mCapacity);
}

RecvBuffer::~RecvBuffer()
{
}

void RecvBuffer::Clean()
{
	int32 dataSize = DataSize();
	if (dataSize == 0)
	{
		// 딱 마침 읽기+쓰기 커서가 동일한 위치라면, 둘 다 리셋.
		mReadPos = mWritePos = 0;
	}
	else
	{
		// 여유 공간이 버퍼 1개 크기 미만이면, 데이터를 앞으로 땅긴다.
		if (FreeSize() < mBufferSize)
		{
			::memcpy(&mBuffer[0], &mBuffer[mReadPos], dataSize);
			mReadPos = 0;
			mWritePos = dataSize;
		}
	}
}

bool RecvBuffer::OnRead(int32 numOfBytes)
{
	if (numOfBytes > DataSize())
		return false;

	mReadPos += numOfBytes;
	return true;
}

bool RecvBuffer::OnWrite(int32 numOfBytes)
{
	if (numOfBytes > FreeSize())
		return false;

	mWritePos += numOfBytes;
	return true;
}
//
ProcessPacket::ProcessPacket()
{
}

void ProcessPacket::Process(BYTE* buffer, int32 len)
{
	PacketHeader header;
	::memcpy(&header, buffer, sizeof(PacketHeader));

	BYTE* payload = buffer + sizeof(PacketHeader);
	int32 payloadSize = header.Size - sizeof(PacketHeader);

	switch (header.PacketType)
	{
	case PKT_Type::KSYNC:
		//ProcessSyncPacket(buffer, len);
		break;
	case PKT_Type::KINPUT:
		//ProcessInputPacket(buffer, len);
		break;
	case PKT_Type::KACTION:
		//ProcessActionPacket(buffer, len);
		break;
	case PKT_Type::KPOSITION:
		//ProcessPositionPacket(buffer, len);
		break;
	case PKT_Type::KMSG:
		//ProcessMsgPacket(buffer, len);
		break;
	default:
		LOG_ERROR("Unknown Packet Type: {}", static_cast<uint32>(header.PacketType));
		break;
	}

}

InputCommand* ProcessPacket::PopCommand()
{
	std::lock_guard<std::mutex> lock(mPopMutex);
	if (!mCommandQueue.empty())
	{
		InputCommand* cmd = mCommandQueue.front();
		mCommandQueue.pop();
		return cmd;
	}
}
