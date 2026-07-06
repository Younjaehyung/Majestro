#pragma once
#include "System.h"
#include "NetIdMap.h"
#include "Network.h"
class  EventManager;

enum class InputButtons : uint8 {
    NONE,
    SPACE,
    SHIFT,
    Q,
    E,
    ATTACK,
    SKILL1,
    SKILL2,
    RELOAD,
    SPECIAL,
    DANCE1,
    END
};

enum class InputMouse : uint8 {
    NONE,
    LEFT,
    RIGHT,
    WHEEL,
    END
};

// 주기적 전송 타이머. hz로 초기화하면 Tick()이 true를 반환할 때만 전송하면 됨
struct RateLimiter
{
    float Interval;
    float Accumulator = 0.f;

    RateLimiter(float hz) : Interval(1.f / hz) {}

    bool Tick(float dt)
    {
        Accumulator += dt;
        if (Accumulator >= Interval)
        {
            Accumulator -= Interval;
            return true;
        }
        return false;
    }
};

class NetSendSystem : public System
{
public:
	NetSendSystem(World* world);
	virtual ~NetSendSystem() = default;
	void Update(float deltaTime);


    void RequestPendingGameStart()
    {
        mPendingGameStart = true;
        mHasSentGameStart = false;
    }

public:

private:
    void TrySendGameStart();
    void TrySendMovement();       // 이동 입력 주기 전송 (UDP, 30Hz)
    void TrySendScene();          // 씬 전환 요청 (TCP)
    void TrySendSync();           // 시간 동기 ping (TCP, 공유 Song Clock)
    void TrySendActionEvents();   // 이벤트성 입력(점프/공격 등) 즉시 전송 (TCP)
    void TrySendRoomEvents();          // 로비 Room : Ready/Character 변경 이벤트를 서버 패킷으로
    void TrySendRoomBrowserEvents();   // 로비 Room : 방 생성/입장/목록/나가기 이벤트
    uint32 GetCurrentRoomId() const;   // LobbyRoomListComponent.mCurrentRoomId (없으면 0)


    void FillCameraFields(float& outPosX, float& outPosY, float& outPosZ, float& outDirX, float& outDirY, float& outDirZ);

    // 패킷 헤더에서 타입을 읽어 SendBuffer에 Push
    template<typename T>
    void SendPacket(const T& pkt)
    {
        SendRequest req{};
        req.Type = reinterpret_cast<const PacketHeader*>(&pkt)->PacketType;
        req.SIze = sizeof(T);
        req.StoreAs(pkt);
        gSendBuffer.Push(req);
    }

	bool mHasSentGameStart = false;     // 로컬 플레이어가 씬에 들어왔을 때 서버에 스폰 요청을 보냈는지 여부
	bool mPendingGameStart = false;     // 로컬 플레이어가 씬에 들어왔을 때 서버에 스폰 요청을 보내기 위해 대기

    RateLimiter mMovementRate{ 30.f };  // 이동 입력 30Hz
    RateLimiter mSyncRate{ 4.f };       // 시간 동기 ping 4Hz


    // 이전 프레임 버튼 상태 (새로 눌린 버튼 감지용)
    uint32 mPrevButtons = 0;
    uint32 mMoveSeq = 0;
};

