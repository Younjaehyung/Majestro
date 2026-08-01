#include "pch.h"
#include "AnimNotifySystem.h"

#include "World.h"
#include "SystemManager.h"
#include "TransformSystem.h"
#include "SocketSystem.h"
#include "VfxSystem.h"

#include "AnimationComponent.h"
#include "CameraComponent.h"
#include "CameraShakeTable.h"
#include "CameraDollyTable.h"
#include "EngineLog.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "SocketComponent.h"
#include "VfxComponent.h"

#include "GameEvents.h"
#include "JsonUtils.h"

AnimNotifySystem::AnimNotifySystem(World* world) : System(world)
{
	mPhase = SysPhase::Post;
}

std::vector<std::type_index> AnimNotifySystem::After() const
{
	return { typeid(TransformSystem), typeid(SocketSystem) };
}

void AnimNotifySystem::Initialize()
{
	// Load shake presets here because animation notifies now own shake timing.
	CameraShakeTable::Load("../Resources/Json/CameraShakeSetting.json");
	CameraDollyTable::Load("../Resources/Json/CameraDollySetting.json");
	LoadTable("../Resources/Json/AnimNotifyTable.json");
}

VfxSystem* AnimNotifySystem::GetVfxSystem()
{
	if (mVfxSystem == nullptr && mWorld != nullptr)
		mVfxSystem = mWorld->GetSystemManager()->GetSystem<VfxSystem>();
	return mVfxSystem;
}

void AnimNotifySystem::Update(float deltaTime)
{
	(void)deltaTime;

	if (mWorld != nullptr && mWorld->HasComponentPool<AnimNotifyComponent>())
	{
		auto view = mWorld->View<AnimNotifyComponent>();
		for (Entity entity : view)
		{
			AnimNotifyComponent* notify = mWorld->GetComponent<AnimNotifyComponent>(entity);
			AnimationComponent* anim = mWorld->GetComponent<AnimationComponent>(entity);
			if (notify == nullptr || anim == nullptr || anim->mAnimClips.empty())
				continue;

			ProcessLayer(entity, *anim, /*useUpper=*/false, notify->mLower);
			if (anim->mEnableUpperBodyLayer)
				ProcessLayer(entity, *anim, /*useUpper=*/true, notify->mUpper);
		}
	}

	// follow VFX가 소켓을 따라가도록 매 프레임 위치 갱신 + 종료분 정리
	UpdateActiveFollows();
}

void AnimNotifySystem::ProcessLayer(Entity owner, AnimationComponent& anim, bool useUpper,
	AnimNotifyComponent::LayerTrack& track)
{
	const uint32 clipIdx = useUpper ? anim.mUpperAnimClipIdx : anim.mLowerAnimClipIdx;
	if (clipIdx >= anim.mAnimClips.size())
		return;

	const shared_ptr<Animator>& clip = anim.mAnimClips[clipIdx];
	if (clip == nullptr)
		return;

	const std::wstring& name = clip->GetName();
	const float duration = max(static_cast<float>(clip->mDuration), 0.0001f);
	const uint32 frameCount = max(clip->mClipMeta.NumFrame, 1u);
	const float frameDuration = duration / static_cast<float>(frameCount);
	const float animTime = std::clamp(useUpper ? anim.mUpperUpdateTime : anim.mUpdateTime, 0.0f, duration);
	const float currF = animTime / frameDuration;

	if (name != track.lastClip || currF < track.lastFrameF - 0.0001f)
		track.lastFrameF = -1.0f;

	const auto it = mTable.find(name);
	if (it != mTable.end())
	{
		for (const AnimNotifyEntry& entry : it->second)
		{
			if (entry.useUpperLayer != useUpper)
				continue;   // 다른 부위는 해당 레이어에서만 판정

			const bool useStartFrame =
				entry.kind == AnimNotifyKind::CameraShake ||
				entry.kind == AnimNotifyKind::CameraDolly;
			const uint32 triggerFrame = useStartFrame ? entry.startFrame : entry.frame;
			const float f = static_cast<float>(triggerFrame);
			if (f > track.lastFrameF && f <= currF)   // 1회 재생
				Fire(owner, entry, frameDuration);
		}
	}

	track.lastClip = name;
	track.lastFrameF = currF;
}

bool AnimNotifySystem::ResolveAnchor(Entity owner, const AnimNotifyEntry& entry, Vec3& outPos) const
{
	if (entry.anchor == AnimNotifyAnchor::Socket)
	{
		SocketComponent* sockets = mWorld->GetComponent<SocketComponent>(owner);
		Matrix socketMatrix;
		if (sockets == nullptr || sockets->TryGetSocketWorldMatrix(entry.socketName, socketMatrix) == false)
			return false;
		outPos = Vec3::Transform(entry.offset, socketMatrix);   // 소켓 로컬 오프셋 적용
		return true;
	}

	// PlayerRoot
	TransformComponent* transform = mWorld->GetComponent<TransformComponent>(owner);
	if (transform == nullptr)
		return false;
	outPos = Vec3::Transform(entry.offset, transform->GetWorldMatrix());
	return true;
}

void AnimNotifySystem::Fire(Entity owner, const AnimNotifyEntry& entry, float frameDuration)
{
	// 카메라 흔들림은 위치나 소켓 계산 없이 애니메이션 프레임 구간으로 실행한다.
	if (entry.kind == AnimNotifyKind::CameraShake)
	{
		FireCameraShake(owner, entry, frameDuration);
		return;
	}

	// 카메라 줌아웃(dolly)도 프레임 구간으로만 실행한다.
	if (entry.kind == AnimNotifyKind::CameraDolly)
	{
		FireCameraDolly(owner, entry, frameDuration);
		return;
	}

	Vec3 worldPos;
	if (ResolveAnchor(owner, entry, worldPos) == false)
		return;   // 스킵

	if (entry.kind == AnimNotifyKind::Vfx)
	{
		VfxSystem* vfxSystem = GetVfxSystem();
		if (vfxSystem == nullptr || entry.vfxName.empty())
			return;

		Entity vfxEntity = vfxSystem->PlayOneShot(entry.vfxName, worldPos, entry.rotation, entry.scale);
		if (entry.follow && entry.anchor == AnimNotifyAnchor::Socket && vfxEntity.IsValid())
			mActiveFollows.push_back(ActiveFollow{ vfxEntity, owner, entry.socketName, entry.offset });
	}
	else // Sfx
	{
		if (entry.sfxKey.empty())
			return;

		EvSfxRequest req;
		req.sfxKey = entry.sfxKey;
		req.position = entry.is3dSfx ? worldPos : Vec3::Zero;   // Zero 면 2D 재생
		mWorld->GetEventManager()->Enqueue(req);
	}
}

void AnimNotifySystem::FireCameraShake(
	Entity owner, const AnimNotifyEntry& entry, float frameDuration)
{
	// 원격 캐릭터의 공격 애니메이션은 현재 클라이언트 화면을 흔들지 않는다.
	if (mWorld->GetComponent<LocalPlayerComponent>(owner) == nullptr)
		return;

	if (entry.endFrame <= entry.startFrame)
	{
		EngineLog::WriteTaggedOnce(EngineLog::Domain::DataTable, "anim-notify",
			"shake-bad-range:" + entry.cameraShakePreset,
			"camera shake frame range 오류 preset=", entry.cameraShakePreset,
			" start=", entry.startFrame, " end=", entry.endFrame);
		return;
	}

	const ShakePreset* preset = CameraShakeTable::Find(entry.cameraShakePreset);
	if (preset == nullptr)
	{
		EngineLog::WriteTaggedOnce(EngineLog::Domain::DataTable, "anim-notify",
			"shake-preset-missing:" + entry.cameraShakePreset,
			"camera shake preset 없음 preset=", entry.cameraShakePreset);
		return;
	}

	for (Entity cameraEntity : mWorld->GetEntitiesWithComponent<CameraTypeComponent>())
	{
		CameraTypeComponent* camera = mWorld->GetComponent<CameraTypeComponent>(cameraEntity);
		if (camera != nullptr && camera->mTargetID == owner.GetID())
		{
			// 종료 프레임에서 시작 프레임을 뺀 실제 애니메이션 시간만큼 흔든다.
			const float shakeDuration =
				static_cast<float>(entry.endFrame - entry.startFrame) * frameDuration;
			camera->TriggerShake(preset->mAngles, shakeDuration, preset->mFrequency);
			return;
		}
	}
}

void AnimNotifySystem::FireCameraDolly(
	Entity owner, const AnimNotifyEntry& entry, float frameDuration)
{
	// 원격 캐릭터의 스킬은 현재 클라이언트 카메라를 움직이지 않는다.
	if (mWorld->GetComponent<LocalPlayerComponent>(owner) == nullptr)
		return;

	if (entry.endFrame <= entry.startFrame)
	{
		EngineLog::WriteTaggedOnce(EngineLog::Domain::DataTable, "anim-notify",
			"dolly-bad-range:" + entry.cameraShakePreset,
			"camera dolly frame range 오류 preset=", entry.cameraShakePreset,
			" start=", entry.startFrame, " end=", entry.endFrame);
		return;
	}

	const DollyPreset* preset = CameraDollyTable::Find(entry.cameraShakePreset);
	if (preset == nullptr)
	{
		EngineLog::WriteTaggedOnce(EngineLog::Domain::DataTable, "anim-notify",
			"dolly-preset-missing:" + entry.cameraShakePreset,
			"camera dolly preset 없음 preset=", entry.cameraShakePreset);
		return;
	}

	for (Entity cameraEntity : mWorld->GetEntitiesWithComponent<CameraTypeComponent>())
	{
		CameraTypeComponent* camera = mWorld->GetComponent<CameraTypeComponent>(cameraEntity);
		if (camera != nullptr && camera->mTargetID == owner.GetID())
		{
			// startFrame~endFrame 구간 동안 뒤로 빠진 채 유지하고, 이후 천천히 복귀
			const float holdTime =
				static_cast<float>(entry.endFrame - entry.startFrame) * frameDuration;
			camera->TriggerDolly(preset->mDistance, holdTime, preset->mInSpeed, preset->mOutSpeed);
			return;
		}
	}
}

void AnimNotifySystem::UpdateActiveFollows()
{
	for (auto it = mActiveFollows.begin(); it != mActiveFollows.end();)
	{
		VfxComponent* vfx = mWorld->GetComponent<VfxComponent>(it->vfx);
		TransformComponent* transform = mWorld->GetComponent<TransformComponent>(it->vfx);

		// 풀 반환/종료되면 추적 종료
		if (vfx == nullptr || transform == nullptr || vfx->mFinished || vfx->mInUse == false)
		{
			it = mActiveFollows.erase(it);
			continue;
		}

		SocketComponent* sockets = mWorld->GetComponent<SocketComponent>(it->src);
		Matrix socketMatrix;
		if (sockets != nullptr && sockets->TryGetSocketWorldMatrix(it->socket, socketMatrix))
		{
			const Vec3 p = Vec3::Transform(it->offset, socketMatrix);
			transform->mLocalPosition = p;
			transform->mWorldPosition = p;
			transform->mWorldMatrix = Matrix::CreateTranslation(p);   // 위치만 추적
		}

		++it;
	}
}

void AnimNotifySystem::LoadTable(const std::string& path)
{
	mTable.clear();

	std::ifstream ifs(path);
	if (!ifs)
	{
		EngineLog::WriteTagged(EngineLog::Domain::DataTable, "anim-notify",
			"open-failed path=", path);
		return;
	}

	json root;
	ifs >> root;

	if (!root.is_object())
		return;

	for (auto clipIt = root.begin(); clipIt != root.end(); ++clipIt)
	{
		const json& arr = clipIt.value();
		if (!arr.is_array())
			continue;

		std::vector<AnimNotifyEntry> entries;
		entries.reserve(arr.size());

		for (const json& e : arr)
		{
			if (!e.is_object())
				continue;

			AnimNotifyEntry entry{};
			entry.frame = static_cast<uint32>(GetOptionalFloat(e, "frame", 0.f));
			entry.startFrame = static_cast<uint32>(GetOptionalFloat(e, "startFrame", 0.f));
			entry.endFrame = static_cast<uint32>(GetOptionalFloat(e, "endFrame", 0.f));
			entry.useUpperLayer = GetOptionalBool(e, "useUpper", true);

			const std::string kind = GetOptionalString(e, "kind", "vfx");
			if (kind == "sfx")
				entry.kind = AnimNotifyKind::Sfx;
			else if (kind == "cameraShake")
				entry.kind = AnimNotifyKind::CameraShake;
			else if (kind == "cameraDolly")
				entry.kind = AnimNotifyKind::CameraDolly;
			else
				entry.kind = AnimNotifyKind::Vfx;

			const std::string anchor = GetOptionalString(e, "anchor", "root");
			entry.anchor = (anchor == "socket") ? AnimNotifyAnchor::Socket : AnimNotifyAnchor::PlayerRoot;

			entry.vfxName = s2ws(GetOptionalString(e, "vfx", ""));
			entry.sfxKey = GetOptionalString(e, "sfx", "");
			entry.cameraShakePreset = GetOptionalString(e, "preset", "");
			entry.socketName = GetOptionalString(e, "socket", "");

			if (e.contains("offset"))   entry.offset = ParseVec3ArrayOrObject(e["offset"], 1.f);
			if (e.contains("rotation")) entry.rotation = ParseVec3ArrayOrObject(e["rotation"], 1.f);
			if (e.contains("scale"))    entry.scale = ParseVec3ArrayOrObject(e["scale"], 1.f);

			entry.follow = GetOptionalBool(e, "follow", false);
			entry.is3dSfx = GetOptionalBool(e, "is3d", true);

			entries.push_back(std::move(entry));
		}

		if (!entries.empty())
			mTable.emplace(s2ws(clipIt.key()), std::move(entries));
	}

	EngineLog::WriteTagged(EngineLog::Domain::DataTable, "anim-notify",
		"loaded clip-notify-sets=", mTable.size(), " path=", path);
}
