#pragma once
#include "pch.h"

#pragma pack(push, 1)

struct Packet				// recv 용
{
	uint16 type;
	uint16 size;
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

enum PKT_Type : uint16 {
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
constexpr uint32 MAX_PACKET_SIZE = 128;

///////////////////////////////////////////

struct KServerPacket : public PacketHeader {

	uint16 dummy{}; // 예시 필드

	KServerPacket() : PacketHeader{ sizeof(KServerPacket), PKT_Type::KSERVER, 0.0 } {}
};

struct SyncPacketData : public PacketHeader {
	uint32_t clientId;
	float    rhythmTime;

	SyncPacketData() : PacketHeader{ sizeof(SyncPacketData), PKT_Type::KSYNC, 0.0 } {}
	SyncPacketData(uint32_t id, float time)
		: PacketHeader{ sizeof(SyncPacketData), PKT_Type::KSYNC, 0.0 }, clientId(id), rhythmTime(time) {
	}
};

struct MovePacketData {
	uint32_t clientId;
	float    x;
	float    y;
	float    z;
};




#pragma pack(pop)