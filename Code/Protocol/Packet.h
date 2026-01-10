#pragma once

#pragma pack(push, 1)

////////////////////////////////////////////

enum PKT_Type : uint32 {
	KNONE = 0,
	PKT_TCP,
	PKT_UDP,
	PKT_LOGIN,
	PKT_SERVER,
	

	// Client -> Server
	C2S_PKT_INPUT,
	C2S_PKT_ACTION,

	// Server -> Client
	S2C_PKT_POS,
	S2C_PKT_SYNC,



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

struct LoginPacket : public PacketTcpHeader {
	uint32 clientId{};
	LoginPacket() : PacketTcpHeader{ sizeof(LoginPacket), PKT_Type::PKT_LOGIN, 0.0 } {}
	LoginPacket(uint32 id)
		: PacketTcpHeader{ sizeof(LoginPacket), PKT_Type::PKT_LOGIN, 0.0 }, clientId(id) {
	}
};

struct ServerPacket : public PacketTcpHeader {

	uint16 dummy{}; // 예시 필드

	ServerPacket() : PacketTcpHeader{ sizeof(ServerPacket), PKT_Type::PKT_SERVER, 0.0 } {}

};

struct S2C_SyncPacket : public PacketTcpHeader {
	uint32_t clientId{};
	float    rhythmTime{};

	S2C_SyncPacket() : PacketTcpHeader{ sizeof(S2C_SyncPacket), PKT_Type::S2C_PKT_SYNC, 0.0 } {}
	S2C_SyncPacket(uint32_t id, float time)
		: PacketTcpHeader{ sizeof(S2C_SyncPacket), PKT_Type::S2C_PKT_SYNC, 0.0 },
		clientId(id), rhythmTime(time) {
	}
};

struct C2S_InputPacket : public PacketUdpHeader {
	
	/*Vec3 Pos;
	Vec3 Dir;
	C2S_InputPacket() : PacketUdpHeader{ sizeof(C2S_InputPacket),C2S_PKT_INPUT,0,0 } {}
	C2S_InputPacket(uint32 id, uint32 seq, Vec3 pos, Vec3 dir)
		: PacketUdpHeader{ sizeof(C2S_InputPacket),C2S_PKT_INPUT,id,seq }, Pos(pos), Dir(dir) {
	}*/
};



#pragma pack(pop)