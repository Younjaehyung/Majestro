#include "pch.h"
#include "UIEmoteFeature.h"

#include "CameraComponent.h"
#include "EmoteDisplayComponent.h"
#include "EmoteWheelStateComponent.h"
#include "Engine.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "TagComponent.h"
#include "Texture.h"
#include "TransformComponent.h"
#include "UIComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "World.h"

namespace
{
	constexpr uint8 EmoteWheelLayer = 15;
	constexpr uint8 EmoteDisplayLayer = 14;
	constexpr float Pi = 3.14159265358979323846f;
	constexpr float TwoPi = Pi * 2.0f;

	constexpr int EmoteAtlasColumns = 5;
	constexpr int EmoteCellSize = 256;
	constexpr int EmoteCount = static_cast<int>(EMOTE_COUNT);

	// 감정표현 UI의 배치, 연출 설정
	constexpr float EmoteWheelRadius = 250.0f;
	constexpr float EmoteWheelIconSize = 150.0f;
	constexpr float EmoteWheelSelectedScale = 1.25f;
	constexpr float EmoteDisplayIconSize = 150.0f;
	constexpr float EmoteDisplayWorldOffsetY = 220.0f;
	constexpr float EmoteDisplayLifetime = 2.5f;
	constexpr float EmoteDisplayPopInTime = 0.15f;
	constexpr float EmoteDisplayFadeOutTime = 0.4f;
}

void UIEmoteFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);
}

void UIEmoteFeature::Update(float dt)
{
	if (mWorld == nullptr)
		return;

	EnsureWheelEntities();
	ConsumeEmoteEvents();
	UpdateWheel();
	UpdateDisplays(dt);
}

void UIEmoteFeature::EnsureWheelEntities()
{
	static_assert(WheelEntityCount == static_cast<size_t>(EMOTE_COUNT),
		"UIEmoteFeature wheel storage must match network emote count");

	const shared_ptr<Texture> emoteTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Emote_Sheet");
	if (emoteTexture == nullptr)
		return;

	for (int index = 0; index < EmoteCount; ++index)
	{
		if (mWheelEntities[index] != NULL_ENTITY)
			continue;

		const float angle = -Pi * 0.5f + TwoPi * static_cast<float>(index) /
			static_cast<float>(EmoteCount);

		Entity iconEntity = mWorld->CreateEntity();
		mWheelEntities[index] = iconEntity;

		UITransformComponent& transform = mWorld->AddComponent<UITransformComponent>(iconEntity);
		transform.mLayoutMode = UILayoutMode::Pixel;
		transform.mAnchor = Anchor::Center;
		transform.mPivot = Vec2(0.5f, 0.5f);
		transform.mPosition = Vec2(
			std::cos(angle) * EmoteWheelRadius,
			std::sin(angle) * EmoteWheelRadius);
		transform.mSize = Vec2(EmoteWheelIconSize, EmoteWheelIconSize);
		transform.mUILayerIndex = EmoteWheelLayer;

		UISpriteComponent& sprite = mWorld->AddComponent<UISpriteComponent>(iconEntity);
		sprite.mTexture = emoteTexture;
		sprite.mVisible = false;
		ApplyEmoteSourceRect(iconEntity, static_cast<uint8>(index));

		mWorld->AddComponent<UIRenderGroupComponent>(iconEntity, UIRenderGroup::Gameplay);
	}
}

void UIEmoteFeature::ConsumeEmoteEvents()
{
	auto eventManager = mWorld->GetEventManager();
	if (!eventManager)
		return;

	eventManager->Consume<EvEmoteTriggered>([this](const EvEmoteTriggered& event)
		{
			if (event.caster.IsValid() && event.emoteId < EmoteCount)
				ShowEmote(event.caster, event.emoteId);
		});
}

void UIEmoteFeature::UpdateWheel()
{
	const EmoteWheelStateComponent* state = mWorld->GetSingleton<EmoteWheelStateComponent>();
	const bool visible = state != nullptr && state->mIsOpen;

	for (int index = 0; index < EmoteCount; ++index)
	{
		Entity iconEntity = mWheelEntities[index];
		UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(iconEntity);
		UITransformComponent* transform = mWorld->GetComponent<UITransformComponent>(iconEntity);
		if (sprite == nullptr || transform == nullptr)
			continue;

		sprite->mVisible = visible;
		if (!visible)
			continue;

		const bool selected = state->mSelectedEmoteId == index;
		const float scale = selected ? EmoteWheelSelectedScale : 1.0f;
		transform->mScale = Vec2(scale, scale);
		sprite->mColorTint = selected
			? Vec4(1.0f, 1.0f, 1.0f, 1.0f)
			: Vec4(0.65f, 0.65f, 0.65f, 0.55f);
	}
}

void UIEmoteFeature::ShowEmote(Entity caster, uint8 emoteId)
{
	if (mWorld->HasComponentPool<EmoteDisplayComponent>())
	{
		for (Entity displayEntity : mWorld->GetEntitiesWithComponent<EmoteDisplayComponent>())
		{
			EmoteDisplayComponent* display = mWorld->GetComponent<EmoteDisplayComponent>(displayEntity);
			if (display == nullptr || display->mCaster != caster)
				continue;
			
			// 새 감정표현이 이미 표시 중인 경우, 기존 표시를 갱신하여 새 감정표현으로 교체
			display->mEmoteId = emoteId;
			display->mElapsed = 0.0f;
			ApplyEmoteSourceRect(displayEntity, emoteId);
			return;
		}
	}

	CreateDisplayEntity(caster, emoteId);
}

Entity UIEmoteFeature::CreateDisplayEntity(Entity caster, uint8 emoteId)
{
	const shared_ptr<Texture> emoteTexture = RESOURCEMANAGER.Get<Texture>(L"UI_Emote_Sheet");
	if (emoteTexture == nullptr)
		return NULL_ENTITY;

	Entity displayEntity = mWorld->CreateEntity();

	EmoteDisplayComponent& display = mWorld->AddComponent<EmoteDisplayComponent>(displayEntity);
	display.mCaster = caster;
	display.mEmoteId = emoteId;
	display.mElapsed = 0.0f;
	display.mLifetime = EmoteDisplayLifetime;

	UITransformComponent& transform = mWorld->AddComponent<UITransformComponent>(displayEntity);
	transform.mLayoutMode = UILayoutMode::Pixel;
	transform.mAnchor = Anchor::TopLeft;
	transform.mPivot = Vec2(0.5f, 0.5f);
	transform.mSize = Vec2(EmoteDisplayIconSize, EmoteDisplayIconSize);
	transform.mUILayerIndex = EmoteDisplayLayer;

	UISpriteComponent& sprite = mWorld->AddComponent<UISpriteComponent>(displayEntity);
	sprite.mTexture = emoteTexture;
	sprite.mVisible = false;
	ApplyEmoteSourceRect(displayEntity, emoteId);

	mWorld->AddComponent<UIRenderGroupComponent>(displayEntity, UIRenderGroup::Gameplay);
	return displayEntity;
}

void UIEmoteFeature::UpdateDisplays(float dt)
{
	if (!mWorld->HasComponentPool<EmoteDisplayComponent>())
		return;

	std::vector<Entity> expiredEntities;
	for (Entity displayEntity : mWorld->GetEntitiesWithComponent<EmoteDisplayComponent>())
	{
		EmoteDisplayComponent* display = mWorld->GetComponent<EmoteDisplayComponent>(displayEntity);
		UITransformComponent* uiTransform = mWorld->GetComponent<UITransformComponent>(displayEntity);
		UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(displayEntity);
		if (display == nullptr || uiTransform == nullptr || sprite == nullptr)
		{
			expiredEntities.push_back(displayEntity);
			continue;
		}

		display->mElapsed += dt;
		if (display->mElapsed >= display->mLifetime)
		{
			expiredEntities.push_back(displayEntity);
			continue;
		}

		TransformComponent* casterTransform = mWorld->GetComponent<TransformComponent>(display->mCaster);
		if (casterTransform == nullptr)
		{
			expiredEntities.push_back(displayEntity);
			continue;
		}

		const Vec3 anchorPosition = casterTransform->mWorldPosition +
			Vec3(0.0f, EmoteDisplayWorldOffsetY, 0.0f);
		Vec2 screenPosition = Vec2::Zero;
		if (!ProjectWorldToScreen(anchorPosition, screenPosition))
		{
			sprite->mVisible = false;
			continue;
		}

		uiTransform->mPosition = screenPosition;
		uiTransform->mFinalPixelPos = screenPosition;

		const float popT = std::clamp(
			display->mElapsed / EmoteDisplayPopInTime, 0.0f, 1.0f);
		const float scale = 0.5f + popT * 0.5f;
		uiTransform->mScale = Vec2(scale, scale);

		const float fadeStart = display->mLifetime - EmoteDisplayFadeOutTime;
		float alpha = 1.0f;
		if (display->mElapsed > fadeStart)
		{
			alpha = 1.0f - std::clamp(
				(display->mElapsed - fadeStart) / EmoteDisplayFadeOutTime,
				0.0f, 1.0f);
		}

		sprite->mVisible = true;
		sprite->mColorTint = Vec4(1.0f, 1.0f, 1.0f, alpha);
	}

	for (Entity entity : expiredEntities)
		mWorld->DestroyEntity(entity);
}

void UIEmoteFeature::ApplyEmoteSourceRect(Entity entity, uint8 emoteId)
{
	UISpriteComponent* sprite = mWorld->GetComponent<UISpriteComponent>(entity);
	if (sprite == nullptr || emoteId >= EmoteCount)
		return;

	const int column = static_cast<int>(emoteId) % EmoteAtlasColumns;
	const int row = static_cast<int>(emoteId) / EmoteAtlasColumns;
	sprite->SetSourceRect(
		static_cast<float>(column * EmoteCellSize),
		static_cast<float>(row * EmoteCellSize),
		static_cast<float>(EmoteCellSize),
		static_cast<float>(EmoteCellSize));
}

bool UIEmoteFeature::ProjectWorldToScreen(const Vec3& worldPosition, Vec2& outScreenPosition) const
{
	if (!mWorld->HasComponentPool<MainCameraComponent>())
		return false;

	const std::vector<Entity> cameraEntities = mWorld->GetEntitiesWithComponent<MainCameraComponent>();
	if (cameraEntities.empty())
		return false;

	CameraComponent* camera = mWorld->GetComponent<CameraComponent>(cameraEntities[0]);
	if (camera == nullptr)
		return false;

	const WindowInfo& window = RENDERMANAGER.GetWindow();
	const DirectX::XMVECTOR world = DirectX::XMVectorSet(
		worldPosition.x, worldPosition.y, worldPosition.z, 1.0f);
	const DirectX::XMVECTOR view = DirectX::XMVector4Transform(world, camera->GetViewMatrix());
	const DirectX::XMVECTOR clipPosition =
		DirectX::XMVector4Transform(view, camera->GetProjectionMatrix());

	DirectX::XMFLOAT4 clip;
	DirectX::XMStoreFloat4(&clip, clipPosition);
	if (clip.w <= 0.0001f)
		return false;

	const float ndcX = clip.x / clip.w;
	const float ndcY = clip.y / clip.w;
	if (ndcX < -1.1f || ndcX > 1.1f || ndcY < -1.1f || ndcY > 1.1f)
		return false;

	outScreenPosition.x = (ndcX * 0.5f + 0.5f) * static_cast<float>(window.Width);
	outScreenPosition.y = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(window.Height);
	return true;
}
