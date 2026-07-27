#pragma once
#include "Component.h"

// PreparePhase 대기 중 플레이어별 준비 상태.
// 클라가 씬 진입 연출(인트로 시네마틱 + 뒤따르는 컷인)을 끝까지 재생하면
// C2S_PKT_INTRO_DONE 을 보내고, NetRecvSystem 이 해당 플레이어 엔티티에 이 값을 세운다.
// 엔티티가 파괴되면(디스커넥트/씬 이동) 함께 정리되므로 세션 맵 누수가 없다.
class PrepareReadyComponent : public Component<PrepareReadyComponent>
{
public:
	bool mIntroDone = false;	// 씬 진입 연출 재생 완료 보고를 받았는지
};
