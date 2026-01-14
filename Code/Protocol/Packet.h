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
	S2C_PKT_RESPAWN,
	S2C_PKT_MOVE,


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

enum class PrefabType : uint8 {
	NONE,
	PLAYER,
	TERRAIN,
	SKYBOX,
	DIRLIGHT,
	COUNT
};

enum class MsgKind : uint8
{
	KNONE = 0,
	ReplicationDelta,
	Spawn,
	Despawn,
};

enum class RepCompKind : uint8
{
	KNONE = 0,
	NetTransform,
	NetHealth,
};

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

///////////////Server To Client///////////////

struct S2C_SyncPacket : public PacketTcpHeader {
	uint32_t clientId{};
	float    rhythmTime{};

	S2C_SyncPacket() : PacketTcpHeader{ sizeof(S2C_SyncPacket), PKT_Type::S2C_PKT_SYNC, 0.0 } {}
	S2C_SyncPacket(uint32_t id, float time)
		: PacketTcpHeader{ sizeof(S2C_SyncPacket), PKT_Type::S2C_PKT_SYNC, 0.0 },
		clientId(id), rhythmTime(time) {
	}
};

struct S2C_PosPacket : public PacketTcpHeader {
	uint32_t clientId{};
	float    x, y;
	S2C_PosPacket() : PacketTcpHeader{ sizeof(S2C_PosPacket), PKT_Type::S2C_PKT_POS, 0.0 } {}
	S2C_PosPacket(uint32_t id, float posX, float posY)
		: PacketTcpHeader{ sizeof(S2C_PosPacket), PKT_Type::S2C_PKT_POS, 0.0 },
		clientId(id), x(posX), y(posY) {
	}
};

struct S2C_RespawnPacket : public PacketTcpHeader {
	uint64_t netEntityId{};
	PrefabType prefabType{ PrefabType::NONE };

	S2C_RespawnPacket() : PacketTcpHeader{ sizeof(S2C_RespawnPacket), PKT_Type::S2C_PKT_RESPAWN, 0.0 } {}
	S2C_RespawnPacket(uint64_t entityId, PrefabType type)
		: PacketTcpHeader{ sizeof(S2C_RespawnPacket), PKT_Type::S2C_PKT_RESPAWN, 0.0 },
		netEntityId(entityId), prefabType(type) {
	}
};

struct S2C_MovePacket : public PacketUdpHeader {
	uint32_t netEntityId{};
	float    x{}, y{}, z{};
	S2C_MovePacket() : PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 } {}
	S2C_MovePacket(uint32_t id, float posX, float posY, float posZ)
		: PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 },
		netEntityId(id), x(posX), y(posY), z(posZ) {
	}
};

struct S2C_SpawnPacekt : public PacketTcpHeader {
	uint64 netEntityId{};
	PrefabType prefabType{ PrefabType::NONE };
	S2C_SpawnPacekt() : PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_RESPAWN, 0.0 } {}
	S2C_SpawnPacekt(uint64 entityId, PrefabType type)
		: PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_RESPAWN, 0.0 },
		netEntityId(entityId), prefabType(type) {
	}
};

///////////////Client To Server///////////////

struct C2S_InputPacket : public PacketUdpHeader {
	uint64 netEntityId{};
	float x, y;

	C2S_InputPacket() : PacketUdpHeader{ sizeof(C2S_InputPacket), PKT_Type::C2S_PKT_INPUT, 0, 0 }, x(0), y(0) {}
	C2S_InputPacket(uint64 entityId, float posX, float posY)
		: PacketUdpHeader{ sizeof(C2S_InputPacket), PKT_Type::C2S_PKT_INPUT, 0, 0 },
		netEntityId(entityId), x(posX), y(posY) {
	}
};



#pragma pack(pop)