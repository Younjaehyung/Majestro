#pragma once

static uint32 ServerTick = 0;

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
	C2S_GAME_START,
	C2S_SCENE_CHANGE,
	C2S_PKT_LOGOUT,
	C2S_PKT_MOVE,
	C2S_PKT_ACTION,
	C2S_PKT_RHYTHM_CHANGED,


	// Server -> Client
	S2C_PKT_LOGIN,
	S2C_PKT_LOGOUT,			// 아직 안함
	S2C_PKT_SCENE_STATE,
	S2C_PKT_SCENE_CONQUEST,
	S2C_PKT_SCENE_ESCORT,
	S2C_PKT_POS,
	S2C_PKT_SYNC,
	S2C_PKT_SPAWN,
	S2C_PKT_SPAWNS,
	S2C_GAME_START,
	S2C_SCENE_CHANGE_RESULT,
	S2C_PKT_RESPAWN,
	S2C_PKT_MOVE,
	S2C_PKT_STATE,
	S2C_PKT_COLLISION,
	S2C_PKT_BULLET_ACTIVATE,
	S2C_PKT_BULLET_DEACTIVATE,
	S2C_PKT_EFFECT_SPAWN,
	S2C_PKT_HEALTH,
	S2C_PKT_ARMOR,
	S2C_PKT_AMMO,
	S2C_PKT_HIT_CONFIRM,

	KMSG,
};

// Fix: SkillType is shared by server and client so network enum values stay identical.
enum class SkillType : uint8
{
	Default = 0,
	BaseAttack,
	BaseSkill1,
	BaseSkill2,
	BaseSkill3,

	GuitarAttack,
	GuitarSkill1,
	GuitarSkill2,
	GuitarSkill3,

	DrumAttack,
	DrumSkill1,
	DrumSkill2,
	DrumSkill3,

	GuitarAttack_1,
	GuitarAttack_2,
	GuitarAttack_3,

	HornAttack,
	PianoAttack,
	BongoAttack,
	BongoShild,

	Max
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
		Sequence = ++ServerTick;
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
	BULLET,
	HEAL_PACK,
	JUMP_PAD,
	MONSTER_SPAWNER,
	TRUCK,
	COUNT
};

enum class SceneId : uint8
{
	MainMenu =0,
	Lobby,
	Loading,
	FirstGame ,
	SecondGame ,
	VGame,
	LGame,
	End,

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


enum class WavePhaseType : uint8 
{ 
	None = 0,
	Prepare , 
	Conquest, 
	Escort,
	Boss, 
	Fail,
	Clear 
};


enum class ReplicatedActionState : uint8
{
	None = 0,
	Attack1,
	Attack2,
	Skill1,
	Skill2,
	Special,
	Reload,
	RhythmChange,
	Aim,
	Hit,
	Stun,
	Dead
};

enum class ReplicatedMovementMode : uint8
{
	Idle = 0,
	Grounded,
	Airborne,
	Falling,
	Landing,
	Dashing,
	Disabled,
	Dead
};

enum ReplicatedControlFlags : uint8
{
	Control_None = 0,
	Control_CanControlHorizontal = 1 << 0,
	Control_CanControlVertical = 1 << 1
	// 여러 상태가 동시에 true 가능
};

enum class ReplicatedExternalMoveMode : uint8
{
	None = 0,			// 외부 이동 없음
	Additive,			// 플레이어 이동에 외부 속도를 더함
	OverrideXZ,			// XZ 이동을 서버/기믹이 강제로 정함
	OverrideY,			// Y축 이동을 서버/기믹이 강제로 정함
	OverrideAll			// 모든 축 이동을 서버/기믹이 강제로 정함
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

struct S2C_SceneStatePacket : public PacketTcpHeader {
	uint32 clientId{};
	float GameTime = 0.0f; // 게임 진행 시간
	uint8 GamePhase = 0; // 현재 게임 phase (예: 준비, 점령, 호위 등)
	int32 PlayerScore = 0; // 플레이어 점수 (예: 점령 시간, 처치 수 등)
	S2C_SceneStatePacket() : PacketTcpHeader{ sizeof(S2C_SceneStatePacket), PKT_Type::S2C_PKT_SCENE_STATE, 0.0 } {}
	S2C_SceneStatePacket(uint32 id, float gameTime, uint8 gamePhase, int32 score)
		: PacketTcpHeader{ sizeof(S2C_SceneStatePacket), PKT_Type::S2C_PKT_SCENE_STATE, 0.0 },
		clientId(id), GameTime(gameTime), GamePhase(gamePhase), PlayerScore(score) {
	}
};
	

struct S2C_ConquestPacket : public PacketTcpHeader {
	uint32 clientId{};

	int WaveCheckPoint = 0; // 현재 웨이브 체크포인트 번호 (0부터 시작)
	int Wave = 1;          // 현재 웨이브 번호 (1부터 시작)

	float WaveInterval = 0.0f; // 웨이브 점령 감소 간격 (초)
	float WaveTime = 0.0f; // 웨이브 점령 시간

	int PlayerNum = 0; // 플레이어 수
	int EnemyNum = 0; // 적 수

	S2C_ConquestPacket() : PacketTcpHeader{ sizeof(S2C_ConquestPacket), PKT_Type::S2C_PKT_SCENE_CONQUEST, 0.0 } {}
	S2C_ConquestPacket(uint32 id, int waveCheckpoint, int wave, float waveInterval, float waveTime, int playerNum, int enemyNum)
		: PacketTcpHeader{ sizeof(S2C_ConquestPacket), PKT_Type::S2C_PKT_SCENE_CONQUEST, 0.0 },
		clientId(id), WaveCheckPoint(waveCheckpoint), Wave(wave), WaveInterval(waveInterval), WaveTime(waveTime), PlayerNum(playerNum), EnemyNum(enemyNum) {
	}
};

struct S2C_EscortPacket : public PacketTcpHeader {
	uint32 clientId{};
	uint8 RouteId = 0; // 호위 경로 ID
	uint8 EscortStage = 0; // 현재 호위 stage (예: 1, 2, 3 등)
	
	float EscortProgress = 0.0f; // 호위 진행도 (0.0 ~ 1.0)
	float EscortTime = 0.0f; // 게임 진행 시간
	S2C_EscortPacket() : PacketTcpHeader{ sizeof(S2C_EscortPacket), PKT_Type::S2C_PKT_SCENE_ESCORT, 0.0 } {}
	S2C_EscortPacket(uint32 id, uint8 routeId, float progress, float escortTime)
		: PacketTcpHeader{ sizeof(S2C_EscortPacket), PKT_Type::S2C_PKT_SCENE_ESCORT, 0.0 },
		clientId(id), RouteId(routeId), EscortProgress(progress), EscortTime(escortTime) {
	}
};

struct S2C_StartGamePacket : public PacketTcpHeader {
	uint32 clientId{};
	S2C_StartGamePacket() : PacketTcpHeader{ sizeof(S2C_StartGamePacket), PKT_Type::S2C_GAME_START, 0.0 } {}
	S2C_StartGamePacket(uint32 id)
		: PacketTcpHeader{ sizeof(S2C_StartGamePacket), PKT_Type::S2C_GAME_START, 0.0 }, clientId(id) {
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
	float    x{}, y{};
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
	uint8_t lowerStateId{}; 
	uint8_t controlFlags{};
	uint8_t externalMoveMode{};
	uint32_t stateSequence{};
	S2C_StatePacket() : PacketTcpHeader{ sizeof(S2C_StatePacket), PKT_Type::S2C_PKT_STATE, 0.0 } {}
	S2C_StatePacket(uint64_t entityId, uint8_t sId)
		: PacketTcpHeader{ sizeof(S2C_StatePacket), PKT_Type::S2C_PKT_STATE, 0.0 },
		netEntityId(entityId), stateId(sId) {
	}
};

struct S2C_HealthPacket : public PacketTcpHeader {
	uint64_t netEntityId{};
	int32_t currentHp{};
	int32_t maxHp{};

	S2C_HealthPacket() : PacketTcpHeader{ sizeof(S2C_HealthPacket), PKT_Type::S2C_PKT_HEALTH, 0.0 } {}
	S2C_HealthPacket(uint64_t entityId, int32_t hp, int32_t maxHpValue)
		: PacketTcpHeader{ sizeof(S2C_HealthPacket), PKT_Type::S2C_PKT_HEALTH, 0.0 },
		netEntityId(entityId), currentHp(hp), maxHp(maxHpValue) {
	}
};

struct S2C_ArmorPacket : public PacketTcpHeader {
	uint64_t netEntityId{};
	int32_t currentArmor{};
	int32_t maxArmor{};

	S2C_ArmorPacket() : PacketTcpHeader{ sizeof(S2C_ArmorPacket), PKT_Type::S2C_PKT_ARMOR, 0.0 } {}
	S2C_ArmorPacket(uint64_t entityId, int32_t armor, int32_t maxArmorValue)
		: PacketTcpHeader{ sizeof(S2C_ArmorPacket), PKT_Type::S2C_PKT_ARMOR, 0.0 },
		netEntityId(entityId), currentArmor(armor), maxArmor(maxArmorValue) {
	}
};



struct S2C_HitConfirmPacket : public PacketTcpHeader {
	// 공격자(local player)에게만 unicast로 전송되는 히트 확인 패킷.
	uint64_t victimNetEntityId{};
	int32_t  damage{};
	uint8_t  isKill{};
	uint8_t  reserved0{};
	uint16_t reserved1{};

	S2C_HitConfirmPacket() : PacketTcpHeader{ sizeof(S2C_HitConfirmPacket), PKT_Type::S2C_PKT_HIT_CONFIRM, 0.0 } {}
};

struct S2C_AmmoPacket : public PacketTcpHeader {
	uint64  netEntityId{};
	int32_t currentAmmo{};
	int32_t maxAmmo{};

	S2C_AmmoPacket() : PacketTcpHeader{ sizeof(S2C_AmmoPacket), PKT_Type::S2C_PKT_AMMO, 0.0 } {}
	S2C_AmmoPacket(uint64 entityId, int32_t ammo, int32_t maxAmmoValue)
		: PacketTcpHeader{ sizeof(S2C_AmmoPacket), PKT_Type::S2C_PKT_AMMO, 0.0 },
		netEntityId(entityId), currentAmmo(ammo), maxAmmo(maxAmmoValue) {
	}
};
struct S2C_MovePacket : public PacketUdpHeader {
	uint64 netEntityId{};
	float    x{}, y{}, z{};
	float   yaw{}, pitch{};
	float	rx{}, ry{}, rz{}, rw{}; // rotation quaternion
	float    vx{}, vy{}, vz{};
	S2C_MovePacket() : PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 } {}
	S2C_MovePacket(uint64 entityId, float posX, float posY, float posZ, float yawAngle, float pitchAngle)
		: PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 },
		netEntityId(entityId), x(posX), y(posY), z(posZ), yaw(yawAngle), pitch(pitchAngle), rx(0), ry(0), rz(0), rw(1) {
	}
	S2C_MovePacket(uint64 entityId, float posX, float posY, float posZ, float yawAngle, float pitchAngle, float velX, float velY, float velZ)
		: PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 },
		netEntityId(entityId), x(posX), y(posY), z(posZ), yaw(yawAngle), pitch(pitchAngle), vx(velX), vy(velY), vz(velZ), rx(0), ry(0), rz(0), rw(1) {
	}
	S2C_MovePacket(uint64 entityId, float posX, float posY, float posZ, float yawAngle, float pitchAngle, float rotX, float rotY, float rotZ, float rotW)
		: PacketUdpHeader{ sizeof(S2C_MovePacket), PKT_Type::S2C_PKT_MOVE, 0,0 },
		netEntityId(entityId), x(posX), y(posY), z(posZ), yaw(yawAngle), pitch(pitchAngle), rx(rotX), ry(rotY), rz(rotZ), rw(rotW) {
	}
};

struct S2C_SpawnPacekt : public PacketTcpHeader {
	uint32 SessionId{};
	uint64 netEntityId{};
	uint8  isLocalPlayer{};
	uint8  Type{};
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

struct S2C_BulletActivatePacket : public PacketTcpHeader {
	uint64_t ownerNetEntityId{};
	uint64_t bulletNetEntityId{};
	uint16_t bulletGeneration{};
	uint8 bulletType{};
	float x{}, y{}, z{};
	float dirX{}, dirY{}, dirZ{};
	float rotX{}, rotY{}, rotZ{};
	float speed{};
	float lifeTime{};
	float size{};

	S2C_BulletActivatePacket()
		: PacketTcpHeader{ sizeof(S2C_BulletActivatePacket), PKT_Type::S2C_PKT_BULLET_ACTIVATE, 0.0 } {
	}
};

struct S2C_BulletDeactivatePacket : public PacketTcpHeader {
	uint64_t bulletNetEntityId{};
	uint16_t bulletGeneration{};

	S2C_BulletDeactivatePacket()
		: PacketTcpHeader{ sizeof(S2C_BulletDeactivatePacket), PKT_Type::S2C_PKT_BULLET_DEACTIVATE, 0.0 } {
	}
};

struct S2C_EffectSpawnPacket : public PacketTcpHeader {
	uint8 effectType{};
	float x{};
	float y{};
	float z{};
	uint8 reason{};
	float rotX{};
	float rotY{};
	float rotZ{};

	S2C_EffectSpawnPacket()
		: PacketTcpHeader{ sizeof(S2C_EffectSpawnPacket), PKT_Type::S2C_PKT_EFFECT_SPAWN, 0.0 } {
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

struct C2S_StartGamePacket : public PacketUdpHeader {
	uint32 clientId{};
	uint8 playerType{};
	C2S_StartGamePacket() : PacketUdpHeader{ sizeof(C2S_StartGamePacket), PKT_Type::C2S_GAME_START, 0, 0 } {}
	C2S_StartGamePacket(uint32 id, uint8 type)
		: PacketUdpHeader{ sizeof(C2S_StartGamePacket), PKT_Type::C2S_GAME_START, 0, 0 }, clientId(id), playerType(type) {
	}
};

struct C2S_SceneChangePacket : public PacketTcpHeader {
	SceneId targetScene{ SceneId::Lobby };
	uint8 reserved{};
	uint16 reserved2{};
	C2S_SceneChangePacket() : PacketTcpHeader{ sizeof(C2S_SceneChangePacket), PKT_Type::C2S_SCENE_CHANGE, 0.0 } {}
	C2S_SceneChangePacket(SceneId target)
		: PacketTcpHeader{ sizeof(C2S_SceneChangePacket), PKT_Type::C2S_SCENE_CHANGE, 0.0 }, targetScene(target) {
	}
};

struct C2S_LogoutPacket : public PacketTcpHeader {
	uint32 clientId{};
	C2S_LogoutPacket() : PacketTcpHeader{ sizeof(C2S_LogoutPacket), PKT_Type::C2S_PKT_LOGOUT, 0.0 } {}
	C2S_LogoutPacket(uint32 id)
		: PacketTcpHeader{ sizeof(C2S_LogoutPacket), PKT_Type::C2S_PKT_LOGOUT, 0.0 }, clientId(id) {
	}
};

// 이벤트성 입력(점프/공격/스킬 등 새로 눌린 순간)을 TCP로 즉시 전송하는 패킷
// UDP 이동 패킷과 달리 손실 없이 서버에 보장 전달
struct C2S_ActionPacket : public PacketTcpHeader {
	uint64 netEntityId{};
	uint32 Buttons{};   // 새로 눌린 버튼 비트마스크
	float  Yaw{};
	float  Pitch{};

	float  CameraX{};
	float  CameraY{};
	float  CameraZ{};
	float  CameraDirX{};
	float  CameraDirY{};
	float  CameraDirZ{};
	C2S_ActionPacket() : PacketTcpHeader{ sizeof(C2S_ActionPacket), PKT_Type::C2S_PKT_ACTION, 0.0 } {}
};

struct C2S_RhythmChangedPacket : public PacketTcpHeader {
	uint64 netEntityId{};
	uint8 previousRhythm{};
	uint8 changedRhythm{};
	uint8 playerType{};
	uint8 reserved{};
	C2S_RhythmChangedPacket() : PacketTcpHeader{ sizeof(C2S_RhythmChangedPacket), PKT_Type::C2S_PKT_RHYTHM_CHANGED, 0.0 } {}
};


struct C2S_MovePacket : public PacketUdpHeader {
	uint64 netEntityId{};

	uint32   Seq = 0;     // 클라 입력 시퀀스(증가)
	float    Dt = 0.0f;   // 선택: 클라 프레임 dt (서버에서는 보통 무시하거나 참고)
	float    MoveX = 0.0f; // -1~1
	float    MoveY = 0.0f; // -1~1
	float	 MoveZ = 0.0f; // -1~1
	uint32    Buttons = 0;  // 비트플래그 (점프/발사/대시 등)
	float    Yaw = 0.0f;
	float    Pitch = 0.0f;

	float    CameraX = 0.0f;
	float    CameraY = 0.0f;
	float    CameraZ = 0.0f;
	float    CameraDirX = 0.0f;
	float    CameraDirY = 0.0f;
	float    CameraDirZ = 0.0f;

	C2S_MovePacket() : PacketUdpHeader{ sizeof(C2S_MovePacket), PKT_Type::C2S_PKT_MOVE, 0, 0 } {}
};

struct S2C_SceneChangeResultPacket : public PacketTcpHeader {
	SceneId currentScene{ SceneId::Lobby };
	uint8 approved{};
	uint16 reserved{};

	S2C_SceneChangeResultPacket()
		: PacketTcpHeader{ sizeof(S2C_SceneChangeResultPacket), PKT_Type::S2C_SCENE_CHANGE_RESULT, 0.0 } {
	}
	S2C_SceneChangeResultPacket(SceneId current, bool isApproved)
		: PacketTcpHeader{ sizeof(S2C_SceneChangeResultPacket), PKT_Type::S2C_SCENE_CHANGE_RESULT, 0.0 },
		currentScene(current), approved(isApproved ? 1 : 0) {
	}
};



#pragma pack(pop)
