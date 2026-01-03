#pragma once
#include <vector>

/*--------------
	RecvBuffer
----------------*/
class ProcessPacket // RECV QUEUE
{
public:
	ProcessPacket();
	~ProcessPacket() {}
	void Process(BYTE* buffer, int32 len);
	InputCommand* PopCommand();
public:
	void ProcessSyncPacket(BYTE* buffer, int32 len) {};
	void ProcessInputPacket(BYTE* buffer, int32 len) {};
	void ProcessActionPacket(BYTE* buffer, int32 len) {};
private:
	std::queue<InputCommand*>   mCommandQueue;
	SpscRingQueue<Packet*, 100>   mQueue;
};


class RecvBuffer
{
	enum { BUFFER_COUNT = 10 };

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

