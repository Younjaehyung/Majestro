#pragma once
#include "Component.h"
#include "Entity.h"

enum class AnimNotifyKind : uint8 { Vfx = 0, Sfx = 1, CameraShake = 2, CameraDolly = 3 };
enum class AnimNotifyAnchor : uint8 { PlayerRoot = 0, Socket = 1 };

struct AnimNotifyEntry
{
	uint32           frame = 0;                          // 발동 프레임
	uint32           startFrame = 0;                     // 카메라 흔들림 시작 프레임
	uint32           endFrame = 0;                       // 카메라 흔들림 종료 프레임
	AnimNotifyKind   kind = AnimNotifyKind::Vfx;
	AnimNotifyAnchor anchor = AnimNotifyAnchor::PlayerRoot;
	bool             useUpperLayer = true;               // 상체/하체 어느 부위로 판정할지

	std::wstring     vfxName;                            // kind==Vfx : 리소스명(L"VFX_...")
	std::string      sfxKey;                             // kind==Sfx : SfxTable.json 키
	std::string      cameraShakePreset;                  // CameraShakeSetting.json 프리셋 이름
	std::string      socketName;                         // anchor==Socket | Root


	Vec3             offset = Vec3::Zero;                // 소켓/루트 로컬 오프셋
	Vec3             rotation = Vec3::Zero;              // VFX 회전(Euler)
	Vec3             scale = Vec3(1.f);
	bool             follow = false;                     // VFX가 소켓을 수명 동안 추적할지(anchor==Socket 전제)

	bool             is3dSfx = true;                     // SFX 3D 여부(위치 전달)
};

class AnimNotifyComponent : public Component<AnimNotifyComponent>
{
public:

	struct LayerTrack
	{
		std::wstring lastClip;
		float        lastFrameF = -1.f;	// 직전 프레임의 실수 위치
	};

	LayerTrack mLower;
	LayerTrack mUpper;
};
