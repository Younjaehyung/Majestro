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
	C2S_PKT_SYNC,

	// 로비 Room
	C2S_ROOM_READY,
	C2S_ROOM_CHARACTER_SELECT,
	C2S_ROOM_CREATE,
	C2S_ROOM_JOIN,
	C2S_ROOM_LIST,
	C2S_ROOM_LEAVE,


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
	S2C_PKT_DESPAWN,
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
	S2C_PKT_COOLDOWN,
	S2C_PKT_GIMMICK_STATE,
	S2C_PKT_RHYTHM_CHANGED,
	S2C_PKT_BEAT_JUDGEMENT,

	// 로비 Room 시스템 : 방 상태 브로드캐스트 / 자격 오류 응답
	S2C_ROOM_STATE,
	S2C_ROOM_ERROR,
	S2C_ROOM_LIST,
	S2C_ROOM_JOIN_RESULT,   // 생성/입장 공통 결과

	// 서버 내부 신호. 네트워크 스레드에서 로직 스레드 세션에 종료 통지.
	INTERNAL_SESSION_LEAVE,

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
	ComboAttack1,
	ComboAttack2,
	Skill1,
	Skill2,
	Special,
	Reload,
	RhythmChange,
	Aim,
	Hit,
	Stun,
	Dead,
	Count
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

enum class EffectSpawnReason : uint8
{
	Fire = 0,
	CollisionEntity = 1,
	CollisionStatic = 2,
	LifetimeExpired = 3,
	Respawn = 4,
	CheckpointReached = 5, // 화물이 거점(stopPoint)에 도착한 순간
};

// 로비 Room 시스템: 한 방의 최대 인원
static constexpr uint8 ROOM_MAX_PLAYERS = 3;

// 선택 가능한 캐릭터 수
static constexpr uint8 ROOM_CHARACTER_COUNT = 3;


// 한 S2C_ROOM_LIST 패킷에 담을 최대 방 수
static constexpr uint8 ROOM_LIST_MAX_ENTRIES = 12;

// 로비 Room 시스템: 게임 시작 거부 사유 / 잘못된 패킷 등 자격 오류 코드
enum class RoomErrorCode : uint8
{
	None = 0,
	NotHost,             // Host 가 아닌 세션이 게임 시작 요청
	NotAllReady,         // 모든 플레이어가 Ready 가 아님
	NotEnoughPlayers,    // 최소 시작 인원 미만 (1명)
	InvalidRoom,         // 존재하지 않는 roomId 또는 본인 방과 다른 roomId
	RoomFull,            // 정원 초과
	AlreadyInRoom,       // 이미 다른 방에 속함
	RoomInGame,          // 게임 중이라 입장 불가
	CharacterTaken,      // 이미 다른 ready 플레이어가 확정한 캐릭터로 Ready 시도
};

// 한 플레이어 슬롯. sessionId == 0 은 비어 있는 슬롯
struct RoomPlayerSlot {
	uint32 sessionId{};
	uint8  playerType{};
	uint8  ready{};     // 0/1
	uint8  isHost{};    // 0/1
	uint8  reserved{};
};

// 방 목록
struct RoomListEntry {
	uint32 roomId{};
	uint8  playerCount{};
	uint8  maxPlayers{};
	uint8  phase{};      // 0=Waiting, 1=InGame
	uint8  reserved{};
};

// 박자 입력 판정 (공유 Song Clock 기반)
// 판정은 패킷 도착 벽시계가 아니라 입력 순간의 곡 위치(songPos)로 한다
enum class BeatJudgement : uint8 { Miss = 0, Good = 1, Perfect = 2 };

constexpr float kBeatPerfectWindow = 0.08f; // 가장 가까운 박자까지 이내면 : Perfect
constexpr float kBeatGoodWindow = 0.185f; // 그 밖 이내면 Good (160~161bpm 반박자 거의 0.18s)
constexpr float kMaxInputSongPosDrift = 0.5f; // 서버 songPos 와 이보다 벌어지면 치팅/지연 : Miss

// 리듬 전환 look-ahead 박자 수
constexpr int64 kRhythmLookAheadBeats = 2;	// 서버가 (현재 절대박자 + 이 값)에 전환을 스케줄해 오차를 흡수.
constexpr int64 kBeatsPerBar = 16;	// 한 마디의 박자 수

// 리듬(노래 변주) 인덱스
enum class Rhythm : uint8 { Neutral = 0, R1, R2, R3, Count };

// 다음 리듬으로 순환(우클릭 입력).
inline uint8 NextRhythm(uint8 cur) { return static_cast<uint8>((cur + 1) % static_cast<uint8>(Rhythm::Count)); }

// 리듬 값 정규화
inline uint8 NormalizeRhythm(uint8 v) { return static_cast<uint8>(v % static_cast<uint8>(Rhythm::Count)); }

// 박자 판정
inline uint8 ClassifyBeatJudgement(float songPos, float beatSec)
{
	if (beatSec <= 0.f) return static_cast<uint8>(BeatJudgement::Miss);

	float phase = songPos - beatSec * static_cast<float>(static_cast<int64>(songPos / beatSec));

	if (phase < 0.f) phase += beatSec;

	const float err = (phase < beatSec - phase) ? phase : (beatSec - phase);

	// 곡 위치를 가장 가까운 박자와 비교해 판정
	if (err <= kBeatPerfectWindow) return static_cast<uint8>(BeatJudgement::Perfect);
	if (err <= kBeatGoodWindow)    return static_cast<uint8>(BeatJudgement::Good);

	return static_cast<uint8>(BeatJudgement::Miss);
}
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

	uint8 ActiveZoneId = 0; // 현재 활성 점령 구역 번호 (1-based, 0=없음). 클라 배너/구역 VFX 식별용
	float ZoneX = 0.0f; // 활성 점령 구역 월드 좌표
	float ZoneY = 0.0f;
	float ZoneZ = 0.0f;

	int WaveCheckPoint = 0; // 현재 웨이브 체크포인트 번호 (0부터 시작)
	int Wave = 1;          // 현재 웨이브 번호 (1부터 시작)

	float WaveInterval = 0.0f; // 웨이브 점령 감소 간격 (초)
	float WaveTime = 0.0f; // 웨이브 점령 시간
	float RequiredConquestTime = 30.0f; // 서버의 stopPoint waitSeconds 기준 점령 목표 시간

	int PlayerNum = 0; // 플레이어 수
	int EnemyNum = 0; // 적 수

	S2C_ConquestPacket() : PacketTcpHeader{ sizeof(S2C_ConquestPacket), PKT_Type::S2C_PKT_SCENE_CONQUEST, 0.0 } {}
	S2C_ConquestPacket(uint32 id, int waveCheckpoint, int wave, float waveInterval, float waveTime, float requiredConquestTime, int playerNum, int enemyNum)
		: PacketTcpHeader{ sizeof(S2C_ConquestPacket), PKT_Type::S2C_PKT_SCENE_CONQUEST, 0.0 },
		clientId(id), WaveCheckPoint(waveCheckpoint), Wave(wave), WaveInterval(waveInterval), WaveTime(waveTime), RequiredConquestTime(requiredConquestTime), PlayerNum(playerNum), EnemyNum(enemyNum) {
	}
};

struct S2C_EscortPacket : public PacketTcpHeader {
	uint32 clientId{};
	uint8 RouteId = 0; // 호위 경로 ID
	uint8 EscortStage = 0; // 현재 호위 stage (예: 1, 2, 3 등)
	uint8 MoveState = 0; // 호위 대상 이동 상태 (0 = 정지, 1 = 이동 중)
	uint8 StageCount = 0; // 전체 호위 구간 수 (UI 텍스처 칸 수와 일치해야 함)

	float EscortProgress = 0.0f; // 호위 진행도 (전체 경로 거리 기준 0.0 ~ 1.0)
	float StageProgress = 0.0f; // 현재 구간 내 진행도 (0.0 ~ 1.0)
	float EscortTime = 0.0f; // 게임 진행 시간
	uint64 TruckNetId = 0; // 호위 대상 네트워크 ID
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


// 시간 동기 ping
struct C2S_SyncPacket : public PacketTcpHeader {
	uint32_t clientId{};

	C2S_SyncPacket() : PacketTcpHeader{ sizeof(C2S_SyncPacket), PKT_Type::C2S_PKT_SYNC, 0.0 } {}
};

// 서버 권위 곡 시간(songPos)을 클라에 통지 — 공유 Song Clock.
struct S2C_SyncPacket : public PacketTcpHeader {
	uint32_t clientId{};
	float    serverSongPos{};   // 서버 BeatSystem 누적 곡 진행 시간(초)
	double   clientEchoTime{};  // 클라 C2S_SyncPacket 의 SendTime echo

	S2C_SyncPacket() : PacketTcpHeader{ sizeof(S2C_SyncPacket), PKT_Type::S2C_PKT_SYNC, 0.0 } {}
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
	uint64_t attackerNetId{}; // 0 = 환경/힐/DOT 등 어택커 없음. 클라이언트가 이걸로 hit direction 계산.
	int32_t currentHp{};
	int32_t maxHp{};
	uint8_t isCritical{};

	S2C_HealthPacket() : PacketTcpHeader{ sizeof(S2C_HealthPacket), PKT_Type::S2C_PKT_HEALTH, 0.0 } {}
	S2C_HealthPacket(uint64_t entityId, int32_t hp, int32_t maxHpValue, uint64_t attacker = 0, bool critical = false)
		: PacketTcpHeader{ sizeof(S2C_HealthPacket), PKT_Type::S2C_PKT_HEALTH, 0.0 },
		netEntityId(entityId), attackerNetId(attacker), currentHp(hp), maxHp(maxHpValue), isCritical(critical ? 1 : 0) {
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
struct S2C_CooldownPacket : public PacketTcpHeader {
	uint64  netEntityId{};
	uint8   skillSlot{};        // 0=Attack,1=Skill1,2=Skill2,3=Reload
	uint8   pad0{};
	uint16  pad1{};
	float   durationSeconds{};  // 전체 쿨타임 길이(초)
	float   remainingSeconds{}; // 남은 쿨타임(초).

	S2C_CooldownPacket() : PacketTcpHeader{ sizeof(S2C_CooldownPacket), PKT_Type::S2C_PKT_COOLDOWN, 0.0 } {}
	S2C_CooldownPacket(uint64 entityId, uint8 slot, float duration, float remaining)
		: PacketTcpHeader{ sizeof(S2C_CooldownPacket), PKT_Type::S2C_PKT_COOLDOWN, 0.0 },
		netEntityId(entityId), skillSlot(slot), durationSeconds(duration), remainingSeconds(remaining) {
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
	uint8  hasInitialTransform{};
	float x{};
	float y{};
	float z{};
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
	uint8  hasInitialTransform{};
	float x{};
	float y{};
	float z{};
	MsgKind kind = MsgKind::Spawns;
	PrefabType prefabType{ PrefabType::NONE };

	S2C_SpawnsPacekt() : PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 } {}
	S2C_SpawnsPacekt(uint32 sessionId, uint64 entityId, PrefabType type = PrefabType::NONE)
		: PacketTcpHeader{ sizeof(S2C_SpawnPacekt), PKT_Type::S2C_PKT_SPAWN, 0.0 },
		SessionId(sessionId), netEntityId(entityId), prefabType(type) {
	}
};

// 기믹 활성/비활성 통지.
struct S2C_GimmickStatePacket : public PacketTcpHeader {
	uint64 netEntityId{};
	uint8  active{ 1 };	// active=0 : 먹혀서 숨김, active=1 : 쿨다운이 끝나 재등장.

	S2C_GimmickStatePacket() : PacketTcpHeader{ sizeof(S2C_GimmickStatePacket), PKT_Type::S2C_PKT_GIMMICK_STATE, 0.0 } {}
	S2C_GimmickStatePacket(uint64 entityId, bool isActive)
		: PacketTcpHeader{ sizeof(S2C_GimmickStatePacket), PKT_Type::S2C_PKT_GIMMICK_STATE, 0.0 },
		netEntityId(entityId), active(isActive ? 1 : 0) {
	}
};

// 엔티티 제거 통지 (해당 캐릭터를 다른 클라에서 제거)
struct S2C_DespawnPacket : public PacketTcpHeader {
	uint64 netEntityId{};

	S2C_DespawnPacket() : PacketTcpHeader{ sizeof(S2C_DespawnPacket), PKT_Type::S2C_PKT_DESPAWN, 0.0 } {}
	S2C_DespawnPacket(uint64 entityId)
		: PacketTcpHeader{ sizeof(S2C_DespawnPacket), PKT_Type::S2C_PKT_DESPAWN, 0.0 }, netEntityId(entityId) {
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

// Lobby Room

// 방 전체 상태 스냅샷. 변경 시점마다 방의 모든 세션에 브로드캐스트
struct S2C_RoomStatePacket : public PacketTcpHeader {
	uint32 roomId{};
	uint8  playerCount{};
	uint8  maxPlayers{};       // = ROOM_MAX_PLAYERS
	uint8  hostSlotIndex{};    // 0xFF = Host 없음
	uint8  reserved{};
	RoomPlayerSlot slots[ROOM_MAX_PLAYERS]{};

	S2C_RoomStatePacket()
		: PacketTcpHeader{ sizeof(S2C_RoomStatePacket), PKT_Type::S2C_ROOM_STATE, 0.0 } {
	}
};

// Room 자격 오류 (요청자에게만 unicast)
struct S2C_RoomErrorPacket : public PacketTcpHeader {
	uint32 roomId{};
	uint8  errorCode{};   // RoomErrorCode
	uint8  reserved0{};
	uint16 reserved1{};

	S2C_RoomErrorPacket()
		: PacketTcpHeader{ sizeof(S2C_RoomErrorPacket), PKT_Type::S2C_ROOM_ERROR, 0.0 } {
	}
};


// 방 목록 (홀 세션에 unicast/브로드캐스트).
struct S2C_RoomListPacket : public PacketTcpHeader {
	uint8  count{};
	uint8  reserved0{};
	uint16 reserved1{};
	RoomListEntry entries[ROOM_LIST_MAX_ENTRIES]{};
	S2C_RoomListPacket()
		: PacketTcpHeader{ sizeof(S2C_RoomListPacket), PKT_Type::S2C_ROOM_LIST, 0.0 } {
	}
};

// 생성/입장 결과 (요청자에게만 unicast)
struct S2C_RoomJoinResultPacket : public PacketTcpHeader {
	uint32 roomId{};     // 성공 시 배정/입장한 roomId, 실패 시 0
	uint8  success{};    // 0/1
	uint8  errorCode{};  // RoomErrorCode
	uint16 reserved{};
	S2C_RoomJoinResultPacket()
		: PacketTcpHeader{ sizeof(S2C_RoomJoinResultPacket), PKT_Type::S2C_ROOM_JOIN_RESULT, 0.0 } {
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
	float  inputSongPos{}; // 입력 순간의 공유 Song Clock 곡 위치(초) — 박자 판정 기준
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

// 서버가 특정 플레이어의 리듬 변경을 전체 클라에 브로드캐스트 — 원격 플레이어 담당 음악 동기화.
struct S2C_RhythmChangedPacket : public PacketTcpHeader {
	uint64 netEntityId{};
	int64  applyAtBeatIndex{};	// 공유 Song Clock 의 절대 박자 인덱스. 서버, 전 클라가 이 박자에 도달할 때 동시에 전환 적용.
	uint8 previousRhythm{};
	uint8 changedRhythm{};
	uint8 playerType{};
	uint8 reserved{};
	S2C_RhythmChangedPacket() : PacketTcpHeader{ sizeof(S2C_RhythmChangedPacket), PKT_Type::S2C_PKT_RHYTHM_CHANGED, 0.0 } {}
};


// 서버가 행동을 발동한 플레이어에게 unicast 로 보내는 박자 판정 결과
struct S2C_BeatJudgementPacket : public PacketTcpHeader {
	uint64 netEntityId{};
	uint8  judgement{};    // BeatJudgement
	uint8  actionButton{}; // InputButtons (어떤 입력에 대한 판정인지)
	uint16 reserved{};
	S2C_BeatJudgementPacket() : PacketTcpHeader{ sizeof(S2C_BeatJudgementPacket), PKT_Type::S2C_PKT_BEAT_JUDGEMENT, 0.0 } {}
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

// Ready 토글값 (서버는 그대로 적용 후 브로드캐스트)
struct C2S_RoomReadyPacket : public PacketTcpHeader {
	uint32 roomId{};
	uint8  ready{};       // 0/1
	uint8  reserved0{};
	uint16 reserved1{};

	C2S_RoomReadyPacket()
		: PacketTcpHeader{ sizeof(C2S_RoomReadyPacket), PKT_Type::C2S_ROOM_READY, 0.0 } {
	}
};

// 캐릭터 변경 (서버에서 Ready 자동 해제 후 브로드캐스트)
struct C2S_RoomCharacterSelectPacket : public PacketTcpHeader {
	uint32 roomId{};
	uint8  playerType{};
	uint8  reserved0{};
	uint16 reserved1{};

	C2S_RoomCharacterSelectPacket()
		: PacketTcpHeader{ sizeof(C2S_RoomCharacterSelectPacket), PKT_Type::C2S_ROOM_CHARACTER_SELECT, 0.0 } {
	}
};



// Lobby Room

struct C2S_RoomCreatePacket : public PacketTcpHeader {
	uint32 reserved{};   // 옵션 없음 (추후 방 이름)
	C2S_RoomCreatePacket()
		: PacketTcpHeader{ sizeof(C2S_RoomCreatePacket), PKT_Type::C2S_ROOM_CREATE, 0.0 } {}
};

struct C2S_RoomJoinPacket : public PacketTcpHeader {
	uint32 roomId{};     // 입장할 방
	C2S_RoomJoinPacket()
		: PacketTcpHeader{ sizeof(C2S_RoomJoinPacket), PKT_Type::C2S_ROOM_JOIN, 0.0 } {}
};

struct C2S_RoomListPacket : public PacketTcpHeader {
	uint32 reserved{};	// 갱신해달라 패킷
	C2S_RoomListPacket()
		: PacketTcpHeader{ sizeof(C2S_RoomListPacket), PKT_Type::C2S_ROOM_LIST, 0.0 } {}
};

struct C2S_RoomLeavePacket : public PacketTcpHeader {
	uint32 roomId{};	// 나갈래 패킷
	C2S_RoomLeavePacket()
		: PacketTcpHeader{ sizeof(C2S_RoomLeavePacket), PKT_Type::C2S_ROOM_LEAVE, 0.0 } {}
};



#pragma pack(pop)
