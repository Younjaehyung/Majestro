#pragma once
#include "System.h"
#include "NetIdMap.h"

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
    void QueueGameStart();

    void SetCachedPlayerType(uint8 playerType){mCachedPlayerType = playerType;}

public:
	void ConvertInput(SendRequest* seq);
    void SendSceneChange(SceneId targetScene);
private:
    void TrySendGameStart();
    void UpdateCachedPlayerType();

    void TrySendMovement();    // 이동 입력 주기 전송 (UDP)
    void TrySendScene();    // 이동 입력 주기 전송 (UDP)
    void TrySendActionEvents();// 이벤트성 입력(점프/공격 등 새로 눌린 버튼) 즉시 전송 (TCP)

    SendRequest mSendData{};
	C2S_MovePacket mInputPacket{};
    C2S_ActionPacket mActionPkt{};

    bool mHasSentGameStart = false;
    bool mPendingGameStart = false;
    uint8 mCachedPlayerType = 0;

    RateLimiter mMovementRate{ 30.f };  // 이동 입력 30Hz


    // 이전 프레임 버튼 상태 (새로 눌린 버튼 감지용)
    uint32 mPrevButtons = 0;

};

