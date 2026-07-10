#include "pch.h"
#include "UIDialogueFeature.h"
#include "NpcComponent.h"
#include "UITransformComponent.h"
#include "UITextComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Engine.h"

namespace
{
	Entity CreateDialogueSprite(World* world, Anchor anchor, const Vec2& pos, const Vec2& size,
		uint8 layer, const std::wstring& texKey)
	{
		Entity e = world->CreateEntity();
		auto& tr = world->AddComponent<UITransformComponent>(e);
		tr.mAnchor = anchor;
		tr.mPosition = pos;
		tr.mSize = size;
		tr.mPivot = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = layer;

		auto& sp = world->AddComponent<UISpriteComponent>(e,
			RESOURCEMANAGER.Get<Texture>(texKey));
		sp.mVisible = false;
		return e;
	}

	Entity CreateDialogueText(World* world, Anchor anchor, const Vec2& pos, const Vec2& size,
		float outline, const Vec4& color)
	{
		Entity e = world->CreateEntity();
		auto& tr = world->AddComponent<UITransformComponent>(e);
		tr.mAnchor = anchor;
		tr.mPosition = pos;
		tr.mSize = size;
		tr.mPivot = Vec2(0.5f, 0.5f);
		tr.mUILayerIndex = 6;

		auto& txt = world->AddComponent<UITextComponent>(e);
		txt.mText = L"";
		txt.mVisible = false;
		txt.mOutlineThickness = outline;
		txt.mColor.f[0] = color.x;
		txt.mColor.f[1] = color.y;
		txt.mColor.f[2] = color.z;
		txt.mColor.f[3] = color.w;
		return e;
	}
}

void UIDialogueFeature::Initialize(World* world)
{
	UIFeature::Initialize(world);

	// 근접 프롬프트: 화면 중앙 아래
	mPromptText = CreateDialogueText(world, Anchor::Center, Vec2(0.f, 320.f), Vec2(500.f, 60.f),
		2.f, Vec4(1.f, 1.f, 1.f, 1.f));

	// 톡박스 배경: 풀스크린 오버레이 (상단 딤 + 하단 잉크 밴드가 텍스트 영역)
	mTalkBg = CreateDialogueSprite(world, Anchor::Center, Vec2(0.f, 0.f), Vec2(2560.f, 1440.f),
		4, L"UI_NoteMan_Talk_0");
	if (UITransformComponent* talkBgTransform = world->GetComponent<UITransformComponent>(mTalkBg))
	{
		// 전체화면
		talkBgTransform->UseScreenRatioLayout(Vec2(0.f, 0.f), Vec2(1.f, 1.f));
	}

	// 캐릭터 초상화: 좌측 하단 (NPC별 텍스처는 대화 시작 시 교체)
	mPortrait = CreateDialogueSprite(world, Anchor::BottomLeft, Vec2(280.f, -280.f), Vec2(480.f, 480.f),
		5, L"UI_NoteMan_Portrait");

	// 대사 텍스트: 하단 잉크 밴드 안쪽
	mNameText = CreateDialogueText(world, Anchor::BottomLeft, Vec2(760.f, -330.f), Vec2(600.f, 55.f),
		2.f, Vec4(1.f, 0.85f, 0.3f, 1.f));
	mLineText = CreateDialogueText(world, Anchor::BottomCenter, Vec2(0.f, -230.f), Vec2(1700.f, 80.f),
		2.f, Vec4(1.f, 1.f, 1.f, 1.f));
	mHintText = CreateDialogueText(world, Anchor::BottomRight, Vec2(-280.f, -120.f), Vec2(400.f, 40.f),
		1.f, Vec4(0.8f, 0.8f, 0.8f, 1.f));

	// 대화창 UI는 Dialogue 그룹
	for (Entity e : { mTalkBg, mPortrait, mNameText, mLineText, mHintText })
		world->AddComponent<UIRenderGroupComponent>(e, UIRenderGroup::Dialogue);
}

void UIDialogueFeature::Update(float dt)
{
	DialogueStateComponent* state = mWorld->GetSingleton<DialogueStateComponent>();
	UITextComponent* prompt = mWorld->GetComponent<UITextComponent>(mPromptText);
	UITextComponent* name = mWorld->GetComponent<UITextComponent>(mNameText);
	UITextComponent* line = mWorld->GetComponent<UITextComponent>(mLineText);
	UITextComponent* hint = mWorld->GetComponent<UITextComponent>(mHintText);
	if (!state || !prompt || !name || !line || !hint)
		return;

	// 근접 프롬프트 (대화 중에는 숨김)
	const bool showPrompt = !state->mActive && state->mNearbyNpc.IsValid();
	prompt->mVisible = showPrompt;
	if (showPrompt)
	{
		if (NpcComponent* npc = mWorld->GetComponent<NpcComponent>(state->mNearbyNpc))
			prompt->mText = L"[F] " + npc->mName;
	}

	// 대화창 (톡박스 배경 + 초상화 + 텍스트)
	UISpriteComponent* talkBg = mWorld->GetComponent<UISpriteComponent>(mTalkBg);
	UISpriteComponent* portrait = mWorld->GetComponent<UISpriteComponent>(mPortrait);
	if (talkBg)
		talkBg->mVisible = state->mActive;
	if (portrait)
		portrait->mVisible = state->mActive;
	name->mVisible = state->mActive;
	line->mVisible = state->mActive;
	hint->mVisible = state->mActive;
	if (state->mActive)
	{
		if (NpcComponent* npc = mWorld->GetComponent<NpcComponent>(state->mNpc))
		{
			// NPC별 초상화 교체
			if (portrait && !npc->mPortraitKey.empty())
			{
				if (shared_ptr<Texture> tex = RESOURCEMANAGER.Get<Texture>(npc->mPortraitKey))
					portrait->mTexture = tex;
			}

			name->mText = npc->mName;
			if (state->mLineIndex >= 0 && state->mLineIndex < (int32)npc->mDialogueLines.size())
				line->mText = npc->mDialogueLines[state->mLineIndex];

			const bool lastLine = (state->mLineIndex + 1 >= (int32)npc->mDialogueLines.size());
			hint->mText = lastLine ? L"[F] 닫기" : L"[F] 다음";
		}
	}
}
