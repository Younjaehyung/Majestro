#pragma once
#include <vector>
#include "EventManager.h"


enum class SysPhase : uint8
{
    Pre = 0,    // 입력/네트워크 수신/커맨드 적용
    Sim,        // 게임 로직/이동/물리/충돌
    Post,       // 카메라/애니메이션/트랜스폼 정리, 이벤트 마감
    Render,     // 렌더 데이터 빌드/커맨드 기록
    Count
};


class World;

class System{
public:
    System(World* world) : mWorld(world) {}
 
    virtual ~System() = default;

    virtual void Update() {}                  //Render
  
    virtual void Initialize() {}
    virtual void OnWorldBegin() {}   // 월드 시작 시 1회
    virtual void Update(float deltaTime) {}   //Logic
    virtual void OnWorldEnd() {}     // 월드 종료 시 1회
    virtual void Shutdown() {}

    virtual const char* GetName() const { return typeid(*this).name(); }


    virtual SysPhase    GetPhase() const { return mPhase; }
    virtual int         GetOrder() const { return mOrder; } // 낮을수록 먼저 실행

    virtual std::vector<std::type_index> Before() const { return {}; }
    virtual std::vector<std::type_index> After()  const { return {}; }
protected:

	SysPhase mPhase = SysPhase::Sim;
	int mOrder = 0;

    World* mWorld = nullptr;
};
