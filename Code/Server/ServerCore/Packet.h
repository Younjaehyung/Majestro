#pragma once
#include "pch.h"

#pragma pack(push, 1)

////////////////////////////////////////////

enum PKT_Type : uint32 {
	KNONE = 0,
	KLOGIN,
	KSERVER,
	KSYNC,
	KINPUT,
	KACTION,
	KPOSITION,
	KTCP,
	KUDP,
	KMSG,
};

struct PacketHeader {
	uint32 Size;
	PKT_Type PacketType;
	PacketHeader() = default;
	PacketHeader(uint32 size, PKT_Type type)
		: Size(size), PacketType(type) {
	}
};

struct PacketTcpHeader {
	PacketHeader Header;
	double   SendTime;

	PacketTcpHeader() = default;
	PacketTcpHeader(uint32 size, PKT_Type type, double time)
		: Header(size, type),  SendTime(time) {
	}
};

struct PacketUdpHeader {
	PacketHeader Header;
	uint32 SessionId;
	uint32 Sequence;
	PacketUdpHeader() = default;
	PacketUdpHeader(uint32 size, PKT_Type type, uint32 sessId, uint32 seq)
		: Header(size, type), SessionId(sessId), Sequence(seq) {
	}
};

static constexpr uint32 kHeaderSize = sizeof(PacketTcpHeader);
constexpr uint32 MAX_PACKET_SIZE = 128;

///////////////////////////////////////////

struct KLoginPacket : public PacketTcpHeader {
	uint32 clientId{};
	KLoginPacket() : PacketTcpHeader{ sizeof(KLoginPacket), PKT_Type::KLOGIN, 0.0 } {}
	KLoginPacket(uint32 id)
		: PacketTcpHeader{ sizeof(KLoginPacket), PKT_Type::KLOGIN, 0.0 }, clientId(id) {
	}
};

struct KServerPacket : public PacketTcpHeader {

	uint16 dummy{}; // 예시 필드

	KServerPacket() : PacketTcpHeader{ sizeof(KServerPacket), PKT_Type::KSERVER, 0.0 } {}

};

struct SyncPacketData : public PacketTcpHeader {
	uint32_t clientId{};
	float    rhythmTime{};

	SyncPacketData() : PacketTcpHeader{ sizeof(SyncPacketData), PKT_Type::KSYNC, 0.0 } {}
	SyncPacketData(uint32_t id, float time)
		: PacketTcpHeader{ sizeof(SyncPacketData), PKT_Type::KSYNC, 0.0 },
		clientId(id), rhythmTime(time) {
	}
};

struct MovePacketData {
	uint32_t clientId;
	float    x;
	float    y;
	float    z;
};



#pragma pack(pop)