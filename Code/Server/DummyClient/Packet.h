#pragma once
#include "pch.h"
#pragma pack(push, 1)

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

struct SyncPacketData{
	uint32_t clientId{1};
	float    rhythmTime{3};
};

struct MovePacketData {
	uint32_t clientId;
	float    x;
	float    y;
	float    z;
};




#pragma pack(pop)


