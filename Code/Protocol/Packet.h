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
	C2S_PKT_LOGIN,
	C2S_PKT_LOGOUT,
	C2S_PKT_INPUT,
	C2S_PKT_ACTION,


	// Server -> Client
	S2C_PKT_LOGIN,
	S2C_PKT_POS,
	S2C_PKT_SYNC,
	S2C_PKT_SPAWN,
	S2C_PKT_SPAWNS,
	S2C_PKT_RESPAWN,
	S2C_PKT_MOVE,
	S2C_PKT_STATE,
	S2C_PKT_COLLISION,


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

////////////////////////////////////////////
enum class PrefabType : uint8 {
	NONE,
	PLAYER,
	
	TERRAIN,
	SKYBOX,
	DIRLIGHT,
	ENEMY,
	COUNT
};

enum class MsgKind : uint8
{
	KNONE = 0,
	ReplicationDelta,
	Spawn,
	Spawns,
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

struct S2C_LoginPacket : public PacketTcpHeader {
	uint32 clientId{};
	S2C_LoginPacket() : PacketTcpHeader{ sizeof(S2C_LoginPacket), PKT_Type::S2C_PKT_LOGIN, 0.0 } {}
	S2C_LoginPacket(uint32 id)
		: PacketTcpHeader{ sizeof(S2C_LoginPacket), PKT_Type::S2C_PKT_LOGIN, 0.0 }, clientId(id) {
	}
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

struct S2C_StatePacket : public PacketTcpHeader {
	uint64_t netEntityId{};
	uint8_t stateId{};
	S2C_StatePacket() : PacketTcpHeader{ sizeof(S2C_StatePacket), PKT_Type::S2C_PKT_STATE, 0.0 } {}
	S2C_StatePacket(uint64_t entityId, uint8_t sId)
		: PacketTcpHeader{ sizeof(S2C_StatePacket), PKT_Type::S2C_PKT_STATE, 0.0 },
		netEntityId(entityId), stateId(sId) {
	}
};

struct S2C_MovePacket : public PacketUdpHeader {
	uint32_t netEntityId{};
	float    x{}, y{}, z{};
	float   yaw{}, pitch{};
	S2C_MovePacket() : PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 } {}
	S2C_MovePacket(uint32 sessId, uint32 seq, uint32_t entityId, float posX, float posY, float posZ, float rotY, float rotP)
		: PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, sessId, seq },
		netEntityId(entityId), x(posX), y(posY), z(posZ), yaw(rotY), pitch(rotP) {
	}
};

struct S2C_SpawnPacekt : public PacketTcpHeader {
	uint32 SessionId{};
	uint64 netEntityId{};
	uint8  isLocalPlayer{};
	uint8  isPlayerType{};
	MsgKind kind = MsgKind::Spawn;
	PrefabType prefabType{ PrefabType::NONE };

	S2C_SpawnPacekt() : PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 } {}
	S2C_SpawnPacekt(uint32 sessionId, uint64 entityId, PrefabType type = PrefabType::NONE)
		: PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 },
		SessionId(sessionId), netEntityId(entityId), prefabType(type) {
	}
};

struct S2C_SpawnsPacekt : public PacketTcpHeader {
	uint32 SessionId{};
	uint64 netEntityId{};
	uint8  isLocalPlayer{};
	MsgKind kind = MsgKind::Spawns;
	PrefabType prefabType{ PrefabType::NONE };

	S2C_SpawnsPacekt() : PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 } {}
	S2C_SpawnsPacekt(uint32 sessionId, uint64 entityId, PrefabType type = PrefabType::NONE)
		: PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 },
		SessionId(sessionId), netEntityId(entityId), prefabType(type) {
	}
};

struct S2C_CollisionPacket : public PacketTcpHeader {	// 임시

	uint64 netEntityId{};
	bool	bIsColliding{};

	S2C_CollisionPacket() : PacketTcpHeader{ sizeof(S2C_CollisionPacket), PKT_Type::S2C_PKT_COLLISION, 0.0 } {}
	S2C_CollisionPacket(uint64 entityId, bool isColliding)
		: PacketTcpHeader{ sizeof(S2C_CollisionPacket), PKT_Type::S2C_PKT_COLLISION, 0.0 },
		netEntityId(entityId), bIsColliding(isColliding) {
	}
};

///////////////Client To Server///////////////

struct C2S_LoginPacket : public PacketUdpHeader {
	uint32 clientId{};
	C2S_LoginPacket() : PacketUdpHeader{ sizeof(C2S_LoginPacket), PKT_Type::C2S_PKT_LOGIN, 0, 0 } {}
	C2S_LoginPacket(uint32 id)
		: PacketUdpHeader{ sizeof(C2S_LoginPacket), PKT_Type::C2S_PKT_LOGIN, 0, 0 }, clientId(id) {
	}
};

struct C2S_LogoutPacket : public PacketTcpHeader {
	uint32 clientId{};
	C2S_LogoutPacket() : PacketTcpHeader{ sizeof(C2S_LogoutPacket), PKT_Type::C2S_PKT_LOGOUT, 0.0 } {}
	C2S_LogoutPacket(uint32 id)
		: PacketTcpHeader{ sizeof(C2S_LogoutPacket), PKT_Type::C2S_PKT_LOGOUT, 0.0 }, clientId(id) {
	}
};

struct C2S_InputPacket : public PacketUdpHeader {
	uint64 netEntityId{};

	uint32   Seq = 0;     // 클라 입력 시퀀스(증가)
	float    Dt = 0.0f;   // 선택: 클라 프레임 dt (서버에서는 보통 무시하거나 참고)
	float    MoveX = 0.0f; // -1~1
	float    MoveY = 0.0f; // -1~1
	float	 MoveZ = 0.0f; // -1~1
	uint8    Buttons = 0;  // 비트플래그 (점프/발사/대시 등)
	float    Yaw = 0.0f;
	float    Pitch = 0.0f;

	C2S_InputPacket() : PacketUdpHeader{ sizeof(C2S_InputPacket), PKT_Type::C2S_PKT_INPUT, 0, 0 } {}
};



#pragma pack(pop)