#pragma once
#include "pch.h"

#pragma pack(push, 1)

////////////////////////////////////////////


enum PKT_Type : uint32 {
	KNONE = 0,
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

	uint32 ClientId{};


	KServerPacket() : PacketHeader{ sizeof(KServerPacket) ,PKT_Type::KSERVER, 0.0 } {}
	KServerPacket(uint32 id)
		: PacketHeader{ sizeof(KServerPacket), PKT_Type::KSERVER, 0.0 }, ClientId(id) {
	}
};

struct SyncPacketData : public PacketHeader {
	uint32_t clientId{};
	float    rhythmTime{};

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