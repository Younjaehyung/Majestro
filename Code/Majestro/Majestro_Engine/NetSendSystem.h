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
    // 이벤트성 입력(점프/공격 등 새로 눌린 버튼)을 TCP로 즉시 전송
    void TrySendActionEvents();

	C2S_InputPacket mInputPacket{};
    bool mHasSentGameStart = false;
    bool mPendingGameStart = false;
    uint8 mCachedPlayerType = 0;

    // 이동 입력 30Hz 주기 전송용 accumulator
    float mInputAccumulator = 0.0f;
    static constexpr float kInputSendInterval = 1.0f / 30.0f;

    // 이전 프레임 버튼 상태 (새로 눌린 버튼 감지용)
    uint32 mPrevButtons = 0;

};

