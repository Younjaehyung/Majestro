#pragma once
#include "System.h"

static constexpr int kCap = 32;

struct InputFrame
{
    uint32   Seq = 0;     // 클라 입력 시퀀스(증가)
    float    Dt = 0.0f;   // 선택: 클라 프레임 dt (서버에서는 보통 무시하거나 참고)
    float    MoveX = 0.0f; // -1~1
    float    MoveY = 0.0f; // -1~1
	float    MoveZ = 0.0f; // -1~1
    uint8    Buttons = 0;  // 비트플래그 (점프/발사/대시 등)
    float    Yaw = 0.0f;
    float    Pitch = 0.0f;
};

struct InputBuffer
{
 
    InputFrame Buf[kCap]{};
    int Head = 0;
    int Tail = 0;

    bool Push(const InputFrame& f)
    {
        int next = (Tail + 1) % kCap;
        if (next == Head) return false; // full
        Buf[Tail] = f;
        Tail = next;
        return true;
    }

    bool Pop(InputFrame& out)
    {
        if (Head == Tail) return false; // empty
        out = Buf[Head];
        Head = (Head + 1) % kCap;
        return true;
    }
};

class NetRecvSystem : public System
{

public:
	NetRecvSystem(World* world);
	void Update(float dt) override;
	void RecvInput(uint32 sessionId, const C2S_InputPacket& inputFrame);
private:

	void LoginProcess(InputCommand& inputCommand);
	void EnemySpawnProcess(InputCommand& inputCommand);
    void HandleSceneChange(InputCommand& inputCommand);
    bool IsSceneChangeAllowed(SceneId currentScene, SceneId requestedScene) const;
    SceneId GetOrCreateScene(uint32 sessionId);



private:

    vector<uint64> mNetEntityIds{};
    bool mEnemySpawnOnce = true;
    std::unordered_map<uint32, SceneId> mSceneBySession{};

	InputBuffer mInputBuffer;
	InputCommand mInputCommand;
};

