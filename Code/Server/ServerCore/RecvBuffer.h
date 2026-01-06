#pragma once
#include <vector>

/*--------------
	RecvBuffer
	Recv시 패킷을 받는 버퍼
----------------*/
class RecvBuffer	// RECV RING BUFFER
{
	enum { BUFFER_COUNT = 64 };

public:
	RecvBuffer(int32 bufferSize);
	~RecvBuffer();

	void			Clean();
	bool			OnRead(int32 numOfBytes);
	bool			OnWrite(int32 numOfBytes);

	BYTE*			ReadPos() { return &mBuffer[mReadPos]; }
	BYTE*			WritePos() { return &mBuffer[mWritePos]; }
	int32			DataSize() { return mWritePos - mReadPos; }
	int32			FreeSize() { return mCapacity - mWritePos; }

private:
	int32			mCapacity = 0;
	int32			mBufferSize = 0;
	int32			mReadPos = 0;
	int32			mWritePos = 0;
	std::vector<BYTE>	mBuffer;
};

