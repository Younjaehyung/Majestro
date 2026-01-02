#pragma once
#include "pch.h"

#pragma pack(push, 1)

struct Packet				// recv 용
{
	uint16_t type;
	uint16_t size;
	const char* payload; // RecvRingBuffer 내부를 참조
};

struct InputCommand			// 로직쓰레드용
{
	int   sessionId;
	float moveX;
	float moveY;
	bool  action1;
	bool  action2;
};
////////////////////////////////////////////

enum PKT_Type : uint32 {
	KSERVER,
	KSYNC,
	KINPUT,
	KACTION,
	KPOSITION,
	KMSG,
};


struct PacketHeader {
	uint32 Size;
	PKT_Type PacketType;
	double   sendTime;
};
static constexpr uint32 kHeaderSize = sizeof(PacketHeader);
constexpr uint32 MAX_PACKET_SIZE = 1024;

///////////////////////////////////////////

struct SyncPacketData {
	uint32_t clientId;
	float    rhythmTime;
};

struct MovePacketData {
	uint32_t clientId;
	float    x;
	float    y;
	float    z;
};




#pragma pack(pop)