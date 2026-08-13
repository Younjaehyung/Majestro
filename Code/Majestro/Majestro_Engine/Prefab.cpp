#include "pch.h"
#include "Engine.h"
#include "RenderManager.h"
#include "ResourceManager.h"
#include "EngineLog.h"
#include "World.h"
#include "Texture.h"
#include "Component.h"
#include "DecalFactory.h"
#include "TransformComponent.h"
#include "RenderComponent.h"
#include "CameraComponent.h"
#include "DeathCamComponent.h"
#include "LightComponent.h"
#include "TagComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "AnimationComponent.h"
#include "SocketComponent.h"
#include "SocketFollowComponent.h"
#include "ComboComponent.h"
#include "RhythmEmissiveComponent.h"
#include "HighlightComponent.h"
#include "WeaponTrailComponent.h"
#include "AnimNotifyComponent.h"
#include "DashSpeedLineComponent.h"
#include "TerrainComponent.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"
#include "UIComponent.h"
#include "HUDPortraitSlotComponent.h"
#include "HUDSkillSlotComponent.h"
#include "BeatComponent.h"
#include "BeatSystem.h"
#include "GravityComponent.h"
#include "MovementComponent.h"
#include "VfxComponent.h"
#include "NetEntityComponent.h"
#include "NetTransformComponent.h"
#include "BoxColliderComponent.h"
#include "DecalComponent.h"
#include "UITextComponent.h"
#include "BulletComponent.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"
#include "GameRuleComponent.h"


Prefab::Prefab() : Object(OBJECT_TYPE::PREFAB)
{
}

Prefab::~Prefab()
{
}

Entity PrefabFactory::BuildWorldMarkerPrefab(World* world, const InputCommand& ctx, const wchar_t* effectName, const Vec3& scale)
{
	Entity entity = world->CreateEntity();

	TransformComponent transform{};
	if (const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>())
	{
		if (spawnPacket->hasInitialTransform != 0)
		{
			transform.mLocalPosition = Vec3(spawnPacket->x, spawnPacket->y, spawnPacket->z);
			transform.mWorldPosition = transform.mLocalPosition;
			transform.mWorldMatrix = Matrix::CreateTranslation(transform.mLocalPosition);
		}
	}
	world->AddComponent<TransformComponent>(entity, transform);

	auto& netComp = world->AddComponent<NetEntityComponent>(entity);
	netComp.mOwnerEntity = entity;

	if (const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>())
	{
		netComp.mNetEntityId = spawnPacket->netEntityId;
		world->NetIdBinding(netComp.mNetEntityId, entity);
	}

	VfxComponent& vfxComp = world->AddComponent<VfxComponent>(entity);
	vfxComp.mVfx = RESOURCEMANAGER.Get<Vfx>(effectName);
	vfxComp.mIsLoop = true;
	vfxComp.mRestartWhenFinished = true;
	vfxComp.mScale = scale;
	return entity;
}


PlayerPrefab::PlayerPrefab(World* world)
{
	mEntityID = world->CreateEntity();
	TransformComponent t{};
	Entity testCamera = world->CreateEntity();
	world->AddComponent<MainCameraComponent>(testCamera);
	world->AddComponent<CameraComponent>(testCamera);
	world->AddComponent<TransformComponent>(testCamera, t);
	world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);
	world->AddComponent<DeathCamComponent>(testCamera);

	//FBX File's Mesh [Naming Convention : SM_(Meshname)_(parts)]
	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");

	std::vector<shared_ptr<Material>> material2s;

	//FBX File's Material [Nameing Convention : (filename)_(0~3)]
	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Idle0");

	
	material2s.push_back(material2);
	t.mLocalPosition = { -8002.9f, 1027.2f, -12519.6f };
	t.mLocalScale = { 10.f, 10.f, 10.f };

	//FBX File's Animation [Naming Convention : Anim_(Name)_(Animationtype)]
	vector<shared_ptr<Animator>> anmators0;
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Walk"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_fall"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Land"));
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));//dash
	anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//dash


	world->AddComponent<ControllerComponent>(mEntityID, t);
	world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, PlayerType::Ibanix);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);
	world->AddComponent<BeatComponent>(mEntityID);
	world->AddComponent<GravityComponent>(mEntityID);
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetEntityComponent>(mEntityID);
	{
		auto& dashLine = world->AddComponent<DashSpeedLineComponent>(mEntityID);
		dashLine.mSourceEntity = mEntityID;
		dashLine.mAutoActivateOnDash = true;
	}
	world->AddComponent<HealthComponent>(mEntityID, 100, 100);
	auto& hpBar = world->AddComponent<UIHpBarComponent>(mEntityID, 180.f, mEntityID, Vec3(0.f, 200.f, 0.f), 20.f);
	// 머리 기준점에서 HP바 높이와 여백만큼 화면 픽셀로 올려 거리 변화에도 위치가 고정되게 하는 값.
	hpBar.mScreenOffsetPx = Vec2(0.f, -(hpBar.mHeight + 8.f));

	Vec3 half{ 10,10,10 };
	Vec3 center{ 0,10,0 };
	world->AddComponent<BoxColliderComponent>(mEntityID,half,center);

}

PlayerPrefab::~PlayerPrefab()
{
}

Entity PlayerPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	
	

	shared_ptr<Mesh> phereMesh;

	shared_ptr<Material> material2;
	std::vector<shared_ptr<Material>> material2s;
	
	vector<shared_ptr<Animator>> anmators0;


	auto& socket = world->AddComponent<SocketComponent>(mEntityID);

	// 리듬 변경 시 이미시브 / 궁극기 이미시브
	world->AddComponent<RhythmEmissiveComponent>(mEntityID);
	world->AddComponent<HighlightComponent>(mEntityID);

	// 캐릭터별 트레일 룩(검기 모양 + 색)
	struct TrailLook
	{
		WeaponTrailVisualStyle style;
		Vec3   coreColor;
		Vec3   edgeColor;
		Vec3   subColor;
		float  intensity;
		uint32 layerCount;
	};

	// 트레일 공통 지오메트리/타이밍
	auto applySlashTrailStyle = [](WeaponTrailComponent& trail, Entity owner,
		const char* tipSocket, const char* baseSocket, uint32 startFrame, uint32 endFrame,
		const TrailLook& look)
		{
			trail.mSourceEntity = owner;
			trail.mSourceType = WeaponTrailSource::Socket;
			trail.mTipSocketName = tipSocket;
			trail.mBaseSocketName = baseSocket;

			// WeaponTrail 이동추적 전환
			trail.mAutoActivateOnAttack = true;	// 공격 상태가 되면 자동으로 트레일 샘플링 시작
			trail.mResetOnActivate = true;      // 매 휘두름마다 이전 샘플 초기화

			trail.mUseAnimationWindow = true;
			trail.mUseUpperAnimationWindow = true;
			trail.mUseAnimationFrameWindow = true;
			trail.mTrailStartFrame = startFrame;
			trail.mTrailEndFrame = endFrame;

			// 공통 지오메트리/타이밍
			trail.mSmoothingSubdivisions = 4;			// 스무딩 4단계 지정.
			trail.mLifetime = 0.14f;
			trail.mSampleInterval = 0.004f;
			trail.mMinSegmentDistance = 1.0f;
			trail.mBaseAlpha = 0.9f;
			trail.mWidthMultiplier = 1.45f;
			// 초승달 폭: 꼬리/머리 양끝을 뾰족하게(작게), 중앙(Mid)을 가장 두껍게.
			trail.mTailWidthScale = 0.10f;
			trail.mMidWidthScale = 1.20f;
			trail.mHeadWidthScale = 2.4f;/*0.08f;*/
			trail.mLayerSpread = 0.22f;
			trail.mSlashCutStrength = 0.55f;
			trail.mSlashLineStrength = 0.7f;
			// 외곽 EdgeColor 발광 / 중심 코어 틴트 강도.
			trail.mSlashEdgeBoost = 1.7f;
			trail.mSlashCoreBoost = 1.0f;
			// 텍스처/결을 길이 방향으로 흘려 에너지가 흐르는 베기 궤적 연출.
			trail.mSlashTexScrollSpeed = 0.9f;

			//캐릭터별 룩(검기 모양/색)
			trail.mVisualStyle = look.style;
			trail.mLayerCount = look.layerCount;
			trail.mCoreColor = look.coreColor;
			trail.mEdgeColor = look.edgeColor;
			trail.mSubColor = look.subColor;
			trail.mIntensity = look.intensity;
		};


	const PlayerType playerType = static_cast<PlayerType>(ctx.ViewAs<S2C_SpawnPacekt>()->Type);
	switch (playerType) {// ctx.ViewAs<S2C_SpawnPacekt>()->isPlayerType) {

	case PlayerType::Rudwig:
	{
		world->AddComponent<HealthComponent>(mEntityID, 200, 200);
		world->AddComponent<ArmorComponent>(mEntityID, 200, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Rudwig_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base0");
		
		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Rudwig_Base1");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_BackRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_RightRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_LeftRun"));
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_02"));//combo attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Attack_03"));//combo attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Ultimate"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Reload"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Rhythm"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Rhythm"));//aim
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Die"));//dead
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Dance_01"));//dance1
		// Ultimate intro placeholder: replace only this resource name when the final clip is added.
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Rudwig_Ultimate_Intro"));

		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, PlayerType::Rudwig);


		socket.mSockets.push_back(SocketDef{ "rhand_tip",  "Bip001 Prop1", Matrix::CreateTranslation(Vec3(0.f, 50.f, 120.f)) });	// 오른손 무기 끝
		socket.mSockets.push_back(SocketDef{ "rhand_base", "Bip001 Prop1", Matrix::CreateTranslation(Vec3(0.f, 50.f, 70.f)) });	// 오른손 무기 손잡이
		socket.mSockets.push_back(SocketDef{ "lhand_tip",  "Bip001 Prop2", Matrix::CreateTranslation(Vec3(0.f, 50.f, 120.f)) });	// 왼손 무기 끝
		socket.mSockets.push_back(SocketDef{ "lhand_base", "Bip001 Prop2", Matrix::CreateTranslation(Vec3(0.f, 50.f, 70.f)) });	// 왼손 무기 손잡이

		// Rudwig 검기 (주황)
		const TrailLook rudwigLook = { WeaponTrailVisualStyle::SwordSlash,
			Vec3(1.0f, 0.85f, 0.55f), Vec3(1.0f, 0.42f, 0.08f), Vec3(0.48f, 0.08f, 0.02f), 4.2f, 4 };

		Entity rightHandTrail = world->CreateEntity();	// 왼손 트레일
		applySlashTrailStyle(world->AddComponent<WeaponTrailComponent>(rightHandTrail), mEntityID, "rhand_tip", "rhand_base", 7, 12, rudwigLook);

		Entity leftHandTrail = world->CreateEntity();	// 오른손 트레일
		applySlashTrailStyle(world->AddComponent<WeaponTrailComponent>(leftHandTrail), mEntityID, "lhand_tip", "lhand_base", 18, 22, rudwigLook);
	}
		break;
	case PlayerType::Ibanix:
	{
		world->AddComponent<HealthComponent>(mEntityID, 150, 150);
		world->AddComponent<ArmorComponent>(mEntityID, 50, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Ibanix_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base0");
		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Ibanix_Base1");
		material2s.push_back(material2);
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run")); //forward
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_BackRun")); //backword
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_RightRun")); //right
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_LeftRun")); //left
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//combo attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Attack_01"));//combo attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Ultimate"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Reload"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Rhythm"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Rhythm"));//aim
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Die"));//dead
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Dance_01"));//dance1
		// Ultimate intro placeholder: replace only this resource name when the final clip is added.
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Ibanix_Ultimate_Intro"));
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, PlayerType::Ibanix);
	}
		break;
	case PlayerType::Fanthor:
	{
		world->AddComponent<HealthComponent>(mEntityID, 250, 250);
		world->AddComponent<ArmorComponent>(mEntityID, 50, 0);

		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Fanthor_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base0");

		material2s.push_back(material2);
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Base1");
		material2s.push_back(material2);
		/*material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Fanthor_Idle0");
		material2s.push_back(material2);*/
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Idle"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_BackRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_RightRun"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_LeftRun"));
		//anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Run"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Jump"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Fall"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Land"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_02"));//dash
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_01"));//attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_01"));//attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_02"));//combo attack1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Attack_03"));//combo attack2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_01"));//skill1
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Skill_02"));//skill2
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Ultimate"));//special
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Reload"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Rhythm"));
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Rhythm"));//aim
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Die"));//dead
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Dance_01"));//dance1
		// Ultimate intro placeholder: replace only this resource name when the final clip is added.
		anmators0.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Fanthor_Ultimate_Intro"));
		world->AddComponent<MainPlayerComponent>(mEntityID, "../Resources/Json/TestJson.json", anmators0, PlayerType::Fanthor);



		// Fanthor 검기 (보라)
		const TrailLook fanthorLook = { WeaponTrailVisualStyle::SwordSlash,
			Vec3(0.95f, 0.80f, 1.0f), Vec3(0.60f, 0.15f, 1.0f), Vec3(0.20f, 0.02f, 0.40f), 3.8f, 4 };

		socket.mSockets.push_back(SocketDef{ "weapon_tip",  "Bip001 Prop1", Matrix::CreateTranslation(Vec3(6.f, 159.f, 13.25f)) });		// 칼 끝
		socket.mSockets.push_back(SocketDef{ "weapon_base", "Bip001 Prop1", Matrix::CreateTranslation(Vec3(0.f, 0.f, 0.f)) });		// 칼 손잡이 쪽

		WeaponTrailComponent& fanthorTrail = world->AddComponent<WeaponTrailComponent>(mEntityID);
		applySlashTrailStyle(fanthorTrail, mEntityID, "weapon_tip", "weapon_base", 11, 15, fanthorLook);


		fanthorTrail.mFrameWindowByAnim = {
			{ L"Anim_Fanthor_Attack_01", { 11, 15 } },	// Attack1 / Attack2 / Special
			{ L"Anim_Fanthor_Attack_02", {  9, 13 } },	// ComboAttack1
			{ L"Anim_Fanthor_Attack_03", {  5,  9 } },	// ComboAttack2
		};
	}
		break;
	}



	TransformComponent t{};
	t.mLocalPosition = { -8002.9f, 1027.2f, -12519.6f };
	if (const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>())
	{
		// 서버 PreparePhase에서 확정한 캐릭터별 위치로 플레이어를 생성한다.
		if (spawnPacket->hasInitialTransform != 0)
		{
			t.mLocalPosition = Vec3(spawnPacket->x, spawnPacket->y, spawnPacket->z);
			t.mWorldPosition = t.mLocalPosition;
			t.mWorldMatrix = Matrix::CreateTranslation(t.mLocalPosition);
		}
	}
	world->AddComponent<TransformComponent>(mEntityID, t);
	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	world->AddComponent<AnimationComponent>(mEntityID, anmators0);

	world->AddComponent<AnimNotifyComponent>(mEntityID);


	{
		auto& dashLine = world->AddComponent<DashSpeedLineComponent>(mEntityID);
		dashLine.mSourceEntity = mEntityID;
		dashLine.mAutoActivateOnDash = true;
		dashLine.mTextureName = L"DashSmoke"; // 대쉬 연기/베이퍼 텍스처        

	}

	world->AddComponent<BeatComponent>(mEntityID);
	GravityComponent& grav = world->AddComponent<GravityComponent>(mEntityID);
	grav.mHight = t.mLocalPosition.y + 13.f; // 임시 동기화
	world->AddComponent<PlayerMovementComponent>(mEntityID);
	world->AddComponent<NetTransformComponent>(mEntityID);
	
	if(ctx.ViewAs<S2C_SpawnPacekt>()->isLocalPlayer == 1)
	{
		world->AddComponent<LocalPlayerComponent>(mEntityID);
		render.mCheckFrustum = false;
		
		Entity testCamera = world->CreateEntity();
		world->AddComponent<MainCameraComponent>(testCamera);
		world->AddComponent<CameraComponent>(testCamera);
		world->AddComponent<TransformComponent>(testCamera, t);
		world->AddComponent<CameraTypeComponent>(testCamera, mEntityID.GetID(), THREE_FPS);
		world->AddComponent<DeathCamComponent>(testCamera);

		HUDPortraitPrefab::HUDPortraitPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
		HUDSkillBarPrefab::HUDSkillBarPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type);
		HUDWeaponPrefab::HUDWeaponPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
		HUDMusicPrefab::HUDMusicPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
		HUDHPBarPrefab::HUDHPBarPrefab(world, ctx.ViewAs<S2C_SpawnPacekt>()->Type, mEntityID);
		HUDCrosshairPrefab::HUDCrosshairPrefab(world);

		//{
		//	// ================ [디버깅] 플레이어 실시간 좌표 UI ================
		//	Entity debugTextObj = world->CreateEntity();
		//	auto& dbgTransform = world->AddComponent<UITransformComponent>(debugTextObj);
		//	dbgTransform.mAnchor = Anchor::Center; // 화면 좌측 상단
		//	dbgTransform.mPosition = Vec2(30.f, -30.f);
		//	dbgTransform.mSize = Vec2(400.f, 50.f);
		//	dbgTransform.mUILayerIndex = 15;

		//	world->AddComponent<UITextComponent>(debugTextObj).mText = L"Pos: ";
		//	world->AddComponent<UIScriptComponent>(debugTextObj).mOnUpdate =
		//		[world, mEntityID, debugTextObj](float /*dt*/)
		//		{
		//			TransformComponent* playerTransform = world->GetComponent<TransformComponent>(mEntityID);
		//			UITextComponent* textComp = world->GetComponent<UITextComponent>(debugTextObj);

		//			if (playerTransform && textComp)
		//			{
		//				// x, y, z 좌표를 읽어와 텍스트 갱신
		//				std::wstring posText = L"Player Pos: (" +
		//					std::to_wstring((int)playerTransform->mLocalPosition.x) + L", " +
		//					std::to_wstring((int)playerTransform->mLocalPosition.y) + L", " +
		//					std::to_wstring((int)playerTransform->mLocalPosition.z) + L")";

		//				textComp->mText = posText;
		//			}
		//		};

		//}
	}
	else 
	{
		// 캐릭터별 전용 HP 텍스처
		std::wstring hpBgName  = L"UI_Fanthor_HP_0";
		std::wstring hpBarName = L"UI_Fanthor_HP_1";
		switch (static_cast<PlayerType>(ctx.ViewAs<S2C_SpawnPacekt>()->Type))
		{
		case PlayerType::Rudwig:
			hpBgName = L"UI_Rudwig_HP_0";  hpBarName = L"UI_Rudwig_HP_1";  break;
		case PlayerType::Ibanix:
			hpBgName = L"UI_Ibanix_HP_0";  hpBarName = L"UI_Ibanix_HP_1";  break;
		case PlayerType::Fanthor:
			hpBgName = L"UI_Fanthor_HP_0"; hpBarName = L"UI_Fanthor_HP_1"; break;
		}

		auto& hp = world->AddComponent<UIHpBarComponent>(mEntityID, 384.f, mEntityID, Vec3(0.f, 200.f, 0.f), 384.f / 3.f, hpBgName, hpBarName);

		// 텍스처 여백 보정: UI_<캐릭터>_HP_1 (768x256) 의 바 픽셀 영역
		// X 118~718 / Y 116~140 — 세 캐릭터 공통 (알파 스캔 측정, ±1px)
		hp.mFillUvRangeX = Vec2(118.f / 768.f, 718.f / 768.f);
		hp.mFillUvRangeY = Vec2(116.f / 256.f, 140.f / 256.f);
		// 쉴드(아머) 바: UI_Player_Shield (768x256) 의 바 색 영역 X 120~718 / Y 118~138 을
		// HP 바 하우징에 리맵해 같은 줄 오른쪽에 표시 (적 월드 바와 동일 방식).
		hp.mShieldMaterialName = L"UI_Player_Shield";
		hp.mShieldUvRangeX = Vec2(120.f / 768.f, 718.f / 768.f);
		hp.mShieldUvRangeY = Vec2(118.f / 256.f, 138.f / 256.f);
		hp.mScreenOffsetPx = Vec2(0.f, -(hp.mHeight + 8.f));
		hp.mHitEffectTextureName = L"UI_Player_HP_3";
		hp.mHitEffectCols = 4;
		hp.mHitEffectRows = 1;
		hp.mHitEffectFrameCount = 4;
		hp.mHitEffectHeightScale = 2.0f;
		hp.mHitEffectOffsetPx = Vec2::Zero;

	}


	Vec3 half{ 30,100,30 };
	Vec3 center{ 0,50,0 };
	world->AddComponent<BoxColliderComponent>(mEntityID, half, center);
	

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);

	return mEntityID;
}

EnemyPrefab::EnemyPrefab(World* world)
{
	//mEntityID = world->CreateEntity();

	shared_ptr<Mesh> phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Noteboar_Body");
	std::vector<shared_ptr<Material>> material2s;


	shared_ptr<Material> material2 = RESOURCEMANAGER.Get<Material>(L"SK_NoteBoar_Run0");
	material2s.push_back(material2);
	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 0.5f, 0.5f, 0.5f };
	vector<shared_ptr<Animator>> anmators;
	anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"SM_Noteboar_Body.001|Action"));

	//mWorld->AddComponent<AnimationComponent>(osw, anmators);
	float i, j, k;
	float n = 10;
	for (i = -50; i < 50; i += 10.0f) {
		for (j = -50; j < 50; j += 10.0f) {
			//for (k = -50; k < 50; k += 10.0f) {
			Entity mEntityID = world->CreateEntity();
			t.mLocalPosition = { i * n, 0, j * n };


			world->AddComponent<TransformComponent>(mEntityID, t);
			world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
			world->AddComponent<GravityComponent>(mEntityID);
			auto& enemyAnim = world->AddComponent<AnimationComponent>(mEntityID, anmators);
			enemyAnim.mEnableAimOffset = true; // 피격 움찔(HitReaction) 활성
			world->AddComponent<EnemyComponent>(mEntityID);
			world->AddComponent<EnemyMovementComponent>(mEntityID);
			world->AddComponent<BoxColliderComponent>(mEntityID);


			/*auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
			netComp.mOwnerEntity = mEntityID;
			netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
			world->NetIdBinding(netComp.mNetEntityId, mEntityID);*/


			//}
		}

	}

}

EnemyPrefab::~EnemyPrefab()
{
}

Entity EnemyPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 0.f, 0.f };
	t.mLocalScale = { 1.3f, 1.3f, 1.3f };
	if (const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>())
	{
		if (spawnPacket->hasInitialTransform != 0)
		{
			t.mLocalPosition = Vec3(spawnPacket->x, spawnPacket->y, spawnPacket->z);
			t.mWorldPosition = t.mLocalPosition;
			t.mWorldMatrix = Matrix::CreateTranslation(t.mLocalPosition);
		}
	}

	Vec3 center{ 0,50,0 };
	Vec3 half{ 50,100,50 };

	shared_ptr<Mesh> phereMesh;
	std::vector<shared_ptr<Material>> material2s;
	shared_ptr<Material> material2;
	vector<shared_ptr<Animator>> anmators;


	auto& hp = world->AddComponent<UIHpBarComponent>(mEntityID, 384.f, mEntityID, Vec3(0.f, 200.f, 0.f), 384.f / 4.f, L"UI_Monster_Hp_0", L"UI_Monster_Hp_1");

	{
		// 텍스처 여백 보정: UI_Monster_Hp_1 (1024x256) 의 바 픽셀 영역 X 278~900 / Y 94~114
		hp.mFillUvRangeX = Vec2(278.f / 1024.f, 900.f / 1024.f);
		hp.mFillUvRangeY = Vec2(94.f / 256.f, 114.f / 256.f);
		// 쉴드(아머) 바: UI_Monster_Hp_2 (1024x256) 의 바 색 영역 X 270~640 / Y 62~96.
		// 이 색 영역을 HP 바 하우징(위 X/Y 구간)에 리맵해 같은 줄 오른쪽에 그린다.
		hp.mShieldMaterialName = L"UI_Monster_Hp_2";
		hp.mShieldUvRangeX = Vec2(270.f / 1024.f, 640.f / 1024.f);
		hp.mShieldUvRangeY = Vec2(62.f / 256.f, 96.f / 256.f);
		hp.mScreenOffsetPx = Vec2(0.f, -(hp.mHeight + 8.f));
		hp.mHitEffectTextureName = L"UI_Player_HP_3";
		hp.mHitEffectCols = 4;
		hp.mHitEffectRows = 1;
		hp.mHitEffectFrameCount = 4;
		hp.mHitEffectHeightScale = 2.0f;
		hp.mHitEffectOffsetPx = Vec2::Zero;
	}

	switch (static_cast<EnemyType>(ctx.ViewAs<S2C_SpawnPacekt>()->Type)) {
	case EnemyType::HornMan:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Hornman_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Hornman_Idle0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Attack_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Hornman_Idle"));

		world->AddComponent<HealthComponent>(mEntityID, 100, 100);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	case EnemyType::Slime:

		t.mLocalScale = { 0.7f, 0.7f, 0.7f };

		center = Vec3(0, 50, 0);
		half = Vec3(130, 250, 130);
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Slime");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Slime_Idle0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Slime_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Slime_Attack"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Slime_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Slime_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Slime_Idle"));

		world->AddComponent<HealthComponent>(mEntityID, 100, 100);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	case EnemyType::Pianoman:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Pianoman_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Pianoman_Idle0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Pianoman_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Pianoman_Attack_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Pianoman_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Pianoman_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Pianoman_Idle"));

		world->AddComponent<HealthComponent>(mEntityID, 75, 75);
		world->AddComponent<EnemyComponent>(mEntityID, EnemyType::Pianoman);
		break;
	case EnemyType::Bongoman:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Bongoman_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Bongoman_Idle0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Bongoman_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Bongoman_Attack_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Bongoman_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Bongoman_Shield"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Bongoman_Idle"));
		hp.mWorldOffset = Vec3(0.f, 400.f, 0.f);
		world->AddComponent<ArmorComponent>(mEntityID, 100, 0);
		world->AddComponent<HealthComponent>(mEntityID, 250, 250);
		world->AddComponent<EnemyComponent>(mEntityID, EnemyType::Bongoman);

		center = Vec3(0, 50, 0);
		half = Vec3(100, 200, 100);
		break;
	case EnemyType::Obelisk:

		t.mLocalScale = { 2.3f, 2.3f, 2.3f };
		hp.mWorldOffset = Vec3(0.f, 500.f, 0.f);

		if (shared_ptr<FBXData> obeliskData = RESOURCEMANAGER.Get<FBXData>(L"SM_Obelisk"))
		{
			if (!obeliskData->GetMeshs().empty())
				phereMesh = obeliskData->GetMeshs().front();
			if (!obeliskData->GetMeshMaterials().empty() && !obeliskData->GetMeshMaterials().front().empty())
				material2s = obeliskData->GetMeshMaterials().front();
			else if (!obeliskData->GetMaterials().empty())
				material2s.push_back(obeliskData->GetMaterials().front());
		}

		//t.mLocalScale = { 100.0f, 100.0f, 100.0f };
		world->AddComponent<HealthComponent>(mEntityID, 200, 0);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	case EnemyType::Fly:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Mew_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_Mew_Run0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Mew_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Mew_Attack_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_Mew_Die"));

		world->AddComponent<HealthComponent>(mEntityID, 50, 50);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	case EnemyType::Brass:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_BrassBoss");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_BrassBoss_Run0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Skill_02"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Die"));//die
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Idle"));//none
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Idle"));//none
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Skill_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Skill_02"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Skill_03"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_BrassBoss_Skill_04"));

		t.mLocalScale = { 1.3f, 1.3f, 1.3f };
		world->AddComponent<HealthComponent>(mEntityID, 2500, 2500);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	case EnemyType::Dragon:
		phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_StringDragon_Body");
		material2 = RESOURCEMANAGER.Get<Material>(L"Anim_StringDragon_Run0");
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Run"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Skill_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Die"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Idle"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Idle"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Skill_01"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Skill_02"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Skill_03"));
		anmators.push_back(RESOURCEMANAGER.Get<Animator>(L"Anim_StringDragon_Skill_04"));
		//t.mLocalScale = { 100.0f, 100.0f, 100.0f };
		//hp.mWorldOffset = Vec3(0.f, 420.f, 0.f);
		center = Vec3(0, 120, 0);
		half = Vec3(180, 220, 180);
		world->AddComponent<HealthComponent>(mEntityID, 2500, 2500);
		world->AddComponent<EnemyComponent>(mEntityID, static_cast<uint8>(ctx.ViewAs<S2C_SpawnPacekt>()->Type));
		break;
	}

	if (material2s.empty() && material2)
		material2s.push_back(material2);
	world->AddComponent<TransformComponent>(mEntityID, t);
	world->AddComponent<NetTransformComponent>(mEntityID);
	world->AddComponent<RenderComponent>(mEntityID, phereMesh, material2s);
	const EnemyType enemyType = static_cast<EnemyType>(ctx.ViewAs<S2C_SpawnPacekt>()->Type);
		if (enemyType == EnemyType::Brass || enemyType == EnemyType::Dragon)
			world->RemoveComponent<UIHpBarComponent>(mEntityID);

	if (enemyType != EnemyType::Obelisk ) {
		auto& enemyAnim = world->AddComponent<AnimationComponent>(mEntityID, anmators);
		enemyAnim.mEnableAimOffset = true; // 피격 움찔(HitReaction) 활성
	}
	world->AddComponent<BoxColliderComponent>(mEntityID,half, center);



	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);


		if ( enemyType == EnemyType::Brass || enemyType == EnemyType::Dragon)
		{
			HUDBossHPBarPrefab bossHpBar{ world, mEntityID, static_cast<int>(enemyType) };
		}



	return mEntityID;
}

BulletPrefab::BulletPrefab(World* world)
{
}

BulletPrefab::~BulletPrefab()
{
}

Entity BulletPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity mEntityID = world->CreateEntity();

	TransformComponent t{};
	t.mLocalPosition = { 0.f, 100.f, 0.f };
	t.mLocalScale = { 10.05f, 10.05f, 10.05f };

	world->AddComponent<TransformComponent>(mEntityID, t);
	//world->AddComponent<BoxColliderComponent>(mEntityID);

	auto& bulletComp = world->AddComponent<BulletComponent>(mEntityID);
	bulletComp.Activate(SkillType::Default, 0, 0, 0, t.mLocalPosition, Vec3::Forward, 90.0f, 2.0f, 10.0f);
	bulletComp.Deactivate();

	const S2C_SpawnPacekt* spawnPacket = ctx.ViewAs<S2C_SpawnPacekt>();
	if (spawnPacket == nullptr)
		return mEntityID;

	auto& netComp = world->AddComponent<NetEntityComponent>(mEntityID);
	netComp.mOwnerEntity = mEntityID;
	netComp.mNetEntityId = spawnPacket->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, mEntityID);

	return mEntityID;
}

HealPackPrefab::HealPackPrefab(World* world)
{
	(void)world;
}

HealPackPrefab::~HealPackPrefab()
{
}

Entity HealPackPrefab::Build(World* world, const InputCommand& ctx)
{

	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Heal", Vec3(5.f, 5.f, 5.f));
}

JumpPadPrefab::JumpPadPrefab(World* world)
{
	(void)world;
}

JumpPadPrefab::~JumpPadPrefab()
{
}

Entity JumpPadPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Jump", Vec3(5.f, 5.f, 5.f));
}

MonsterSpawnerMarkerPrefab::MonsterSpawnerMarkerPrefab(World* world)
{
	(void)world;
}

MonsterSpawnerMarkerPrefab::~MonsterSpawnerMarkerPrefab()
{
}

Entity MonsterSpawnerMarkerPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Spawn", Vec3(4.f, 4.f, 4.f));
}


SkyBoxPrefab::SkyBoxPrefab(World* world)
{

	mEntityID = world->CreateEntity();
	TransformComponent bt{};


	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.Get<Mesh>(L"Sphere");

	shared_ptr<Material> skyBoxMat = RESOURCEMANAGER.Get<Material>(L"Skybox");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(skyBoxMat);

	world->AddComponent<TransformComponent>(mEntityID, bt);
	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, skyBoxMesh, materials);
	render.mCheckFrustum = false;

}

SkyBoxPrefab::~SkyBoxPrefab()
{
}

// 해수면 높이(Y)
static constexpr float kSeaLevelY = -350.0f;

OceanPrefab::OceanPrefab(World* world)
{
	mEntityID = world->CreateEntity();

	TransformComponent bt{};
	bt.mLocalScale    = Vec3(30000.f, 1.f, 30000.f); // 맵을 덮는 대형 평면
	bt.mLocalPosition = Vec3(10000.f, kSeaLevelY, 0.f);  // 해수면 높이

	world->AddComponent<TransformComponent>(mEntityID, bt);

	shared_ptr<Mesh> mesh = RESOURCEMANAGER.Get<Mesh>(L"Plane");
	std::vector<shared_ptr<Material>> materials{ RESOURCEMANAGER.Get<Material>(L"Ocean") };

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, mesh, materials);
	render.mCheckFrustum = false; // 항상 보이는 거대 평면
}

OceanPrefab::~OceanPrefab()
{
}

TerrainPrefab::TerrainPrefab(World* world)
{
	mEntityID = world->CreateEntity();

	TransformComponent bt{};

	bt.mLocalScale = Vec3(100, 100, 100);
	//bt.mLocalRotationE = Vec3(0, 90, 0);
	bt.mLocalPosition = Vec3(-0.5 * 378 * 100, -41.6f, -0.5 * 378 * 100);


	world->AddComponent<TransformComponent>(mEntityID, bt);

	// heightmap 512x512 => 타일 511x511


	shared_ptr<Mesh> terrain = RESOURCEMANAGER.LoadTerrainMesh(378, 378);
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");

	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 378, 378, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	std::vector<shared_ptr<Material>> materials{
		
		RESOURCEMANAGER.Get<Material>(L"Grass"),
		RESOURCEMANAGER.Get<Material>(L"Sand_Rock"),
		RESOURCEMANAGER.Get<Material>(L"Dirt"),
		RESOURCEMANAGER.Get<Material>(L"Sand"),
		RESOURCEMANAGER.Get<Material>(L"Dirt_Road"),
		//RESOURCEMANAGER.Get<Material>(L"SnowFootprints"),
		//RESOURCEMANAGER.Get<Material>(L"Soil_Mud") ,
		//RESOURCEMANAGER.Get<Material>(L"Asphalt")
	};

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, terrain, materials);
	render.mCheckFrustum = false;




}

TerrainPrefab::~TerrainPrefab()
{
}

Entity TerrainPrefab::Build(World* world, const InputCommand& ctx)
{

	Entity mEntityID = world->CreateEntity();
	TransformComponent bt{};
	bt.mLocalScale = (Vec3(30.f, 250.f, 30.f));
	bt.mLocalPosition = Vec3(-150.f, -70.f, -150.f);

	shared_ptr<Mesh> skyBoxMesh = RESOURCEMANAGER.LoadTerrainMesh(64, 64);

	// 빌보드 머티리얼(
	shared_ptr<Material> heightMap = RESOURCEMANAGER.Get<Material>(L"Terrain");
	std::vector<shared_ptr<Material>> materials;
	materials.push_back(heightMap);

	world->AddComponent<TransformComponent>(mEntityID, bt);
	TerrainComponent& terrainc = world->AddComponent<TerrainComponent>(mEntityID, 64, 64, heightMap);
	terrainc.mTerrainWorldPosition = bt.mLocalPosition;
	terrainc.mTerrainWorldScale = bt.mLocalScale;

	RenderComponent& render = world->AddComponent<RenderComponent>(mEntityID, skyBoxMesh, materials);
	render.mCheckFrustum = false;

	return mEntityID;
}

DirLightPrefab::DirLightPrefab(World* world, const Vec3& direction, const DirLightColors& colors)
{
	LightComponent l{};
	l.mLightInfo.Position = { Vec3(0, 0, 0) };
	l.SetAmbient(colors.ambient);
	l.SetDiffuse(colors.diffuse);
	l.SetSpecular(colors.specular);
	l.SetLightDirection(direction);
	mEntityID = LightFactory::CreateLight(world, LIGHT_TYPE::DIRECTIONAL_LIGHT, l);
}

DirLightPrefab::~DirLightPrefab()
{
}

BillboardPrefab::BillboardPrefab(World* world)
{
}

BillboardPrefab::~BillboardPrefab()
{
}


HUDPortraitPrefab::HUDPortraitPrefab(World* world, uint8 playerType)
{
	// 슬롯별 위치/크기 (slot0 = 메인 player, slot1/2 =  나머지 player)
	struct SlotLayout { Vec2 pos; Vec2 size; };
	const std::array<SlotLayout, 3> kLayout = { {
		{ Vec2(64.f, -300.f), Vec2(256.f, 256.f) },  // slot0 메인(로컬)
		{ Vec2(64.f, -464.f), Vec2(160.f, 160.f) },  // slot1  나머지 player
		{ Vec2(64.f, -640.f), Vec2(160.f, 160.f) }   // slot2  나머지 player
	} };

	const float BounceAmplitude = 0.05f;
	const float BounceFrequency = 2.f;
	const float BounceDamping   = 10.0f;

	std::wstring hpBgName = L"UI_Fanthor_HP_0";
	std::wstring hpBarName = L"UI_Fanthor_HP_1";


	switch (playerType)
	{
	case PlayerType::Fanthor:
		hpBgName = L"UI_Fanthor_HP_0";
		hpBarName = L"UI_Fanthor_HP_1";
		break;
	case PlayerType::Rudwig:
		hpBgName = L"UI_Rudwig_HP_0";
		hpBarName = L"UI_Rudwig_HP_1";
		break;
	case PlayerType::Ibanix:
		hpBgName = L"UI_Ibanix_HP_0";
		hpBarName = L"UI_Ibanix_HP_1";
		break;
	}

	for (uint8 i = 0; i < kLayout.size(); ++i)
	{
		const SlotLayout& L = kLayout[i];


		auto makeSprite = [&](int layer, bool bounce) -> Entity
		{
			Entity e = world->CreateEntity();
			auto& t = world->AddComponent<UITransformComponent>(e);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = L.pos;
			t.mSize = L.size;
			t.mUILayerIndex = layer;

			auto& sp = world->AddComponent<UISpriteComponent>(e);
			sp.mVisible = false;

			if (bounce)
			{
				auto& m = world->AddComponent<UIActionComponent>(e);
				m.mDuration = 0.5f;
				m.mActor = UIActor::Player;
				m.mState = UIActionState::Bounce;
				m.mIsLoop = true;
				m.mBounceAmplitude = BounceAmplitude;
				m.mBounceFrequency = BounceFrequency;
				m.mBounceDamping = BounceDamping;
			}
			return e;
		};

		Entity back0 = makeSprite(1, false);
		Entity back1 = makeSprite(2, false);
		Entity head0 = makeSprite(3, false);
		Entity head1 = makeSprite(4, true);

		
		auto& slot = world->AddComponent<HUDPortraitSlotComponent>(back0);
		slot.mSlotIndex = i;
		slot.mBack0 = back0;
		slot.mBack1 = back1;
		slot.mHead0 = head0;
		slot.mHead1 = head1;

		// 나머지 플레이어
		if (i >= 1)
		{
			const float  hpScale = 1.f / 3.f;
			const Vec2   hpSize  = Vec2(512.f, 96.f) * hpScale;             // ≈ (170, 32)
			const float  gap     = 12.f;
			const Vec2   hpPos   = Vec2(L.pos.x + L.size.x + gap,           // 초상화 오른쪽
				L.pos.y + L.size.y * 0.5f - hpSize.y * 0.5f);              // 초상화 세로 중앙

			// HP 빈 바 배경
			Entity hpBack = world->CreateEntity();
			{
				auto& t = world->AddComponent<UITransformComponent>(hpBack);
				t.mAnchor = Anchor::BottomLeft;
				t.mPosition = hpPos;
				t.mSize = hpSize;
				t.mPivot = Vec2(0.f, 0.f);
				t.mUILayerIndex = 6;
				auto& sp = world->AddComponent<UISpriteComponent>(hpBack, RESOURCEMANAGER.Get<Texture>(hpBgName));
				sp.mVisible = false;
			}

			// HP 채움 바
			Entity hpFill = world->CreateEntity();
			{
				auto& t = world->AddComponent<UITransformComponent>(hpFill);
				t.mAnchor = Anchor::BottomLeft;
				t.mPosition = hpPos;
				t.mSize = hpSize;
				t.mPivot = Vec2(0.f, 0.f);
				t.mUILayerIndex = 5;
				auto& sp = world->AddComponent<UISpriteComponent>(hpFill, RESOURCEMANAGER.Get<Texture>(hpBarName));
				sp.mVisible = false;
			}

			// 쉴드(아머) 바 — 체력 오른쪽에 이어붙음. 위치/크기는 매 프레임 갱신(HUDPortraitUpdateFeature)
			Entity hpShield = world->CreateEntity();
			{
				auto& t = world->AddComponent<UITransformComponent>(hpShield);
				t.mAnchor = Anchor::BottomLeft;
				t.mPosition = hpPos;
				t.mSize = hpSize;
				t.mPivot = Vec2(0.f, 0.f);
				t.mUILayerIndex = 5;
				auto& sp = world->AddComponent<UISpriteComponent>(hpShield, RESOURCEMANAGER.Get<Texture>(L"UI_Player_Shield"));
				sp.SetSourceRect(118.f, 116.f, 600.f, 24.f);   // 바 픽셀 구간만 크롭
				sp.mVisible = false;
			}

			//// HP 텍스트
			//Entity hpText = world->CreateEntity();
			//{
			//	auto& t = world->AddComponent<UITransformComponent>(hpText);
			//	t.mAnchor = Anchor::BottomLeft;
			//	t.mPosition = Vec2(hpPos.x + hpSize.x * 0.5f, hpPos.y + hpSize.y * 0.5f);
			//	t.mSize = hpSize;
			//	t.mPivot = Vec2(0.5f, 0.5f);
			//	t.mScale = Vec2(hpScale, hpScale);
			//	t.mUILayerIndex = 7;
			//	auto& text = world->AddComponent<UITextComponent>(hpText);
			//	text.mText = L"";
			//	text.mVisible = false;
			//}


			slot.mHpBack = hpBack;
			slot.mHpFill = hpFill;
			slot.mHpShield = hpShield;
			//slot.mHpText = hpText;
		}
	}
}

HUDPortraitPrefab::~HUDPortraitPrefab()
{
}

HUDSkillBarPrefab::HUDSkillBarPrefab(World* world, uint8 playerType)
{

	const Vec2  kSkillSize = Vec2(156.f, 156.f);
	const Vec2  kUltimateSize = kSkillSize * 1.25f;
	constexpr float kRightMargin = 72.f;
	constexpr float kBottomMargin = 64.f;
	constexpr float kSlotGap = 24.f;
	constexpr float kBackPadding = 10.f;

	const float normalY = -(kBottomMargin + kSkillSize.y);
	const float ultimateY = -(kBottomMargin + kUltimateSize.y);
	const float ultimateX = -(kRightMargin + kUltimateSize.x);
	const float skill2X = ultimateX - kSlotGap - kSkillSize.x;
	const float skill1X = skill2X - kSlotGap - kSkillSize.x;


	const std::array<Vec2, 3> kSkillPos = {
		Vec2(skill1X, normalY),       // Skill1 (E)
		Vec2(skill2X, normalY),       // Skill2 (Shift)
		Vec2(ultimateX, ultimateY),   // 궁극기 (Q)
	};

	constexpr uint8 kUltimateSlot = 255;                    // 쿨타임 미추적 슬롯(오버레이 비활성)
	const uint8 kSkillSlotId[3] = { 1, 2, kUltimateSlot };  // Skill1, Skill2, 궁극기

#ifdef _IMGUI
	std::vector<EditorProperty> props;
	Entity imguiOwner = NULL_ENTITY;
#endif

	// 아이콘 아틀라스
	constexpr float kCell = 256.f;
	const float kIconCol[3] = { 0.f, 1.f, 5.f };
	const Vec2  kKeyCell[3] = {
		Vec2(1024.f, 1024.f),   // E    (Skill1)
		Vec2(1280.f, 1024.f),   // Shift(Skill2)
		Vec2( 768.f, 1024.f),   // Q    (궁극기)
	};
	int atlasRow = 3;  // 기본 Rudwig
	switch (playerType)
	{
	case Rudwig:  atlasRow = 3; break;
	case Ibanix:  atlasRow = 2; break;
	case Fanthor: atlasRow = 1; break;
	default:      atlasRow = 3; break;
	}

	shared_ptr<Texture> keyTex   = RESOURCEMANAGER.Get<Texture>(L"UI_SkillIcon_Sheet");
	shared_ptr<Texture> backTex   = RESOURCEMANAGER.Get<Texture>(L"UI_SkillIcon_Sheet");
	shared_ptr<Texture> skiilTex = RESOURCEMANAGER.Get<Texture>(L"UI_SkillIcon_Sheet");
	shared_ptr<Texture> overlayTex = RESOURCEMANAGER.Get<Texture>(L"UI_SkillIcon_Sheet");

	// 박자 바운스
	auto addBeatBounce = [&](Entity e)
	{
		auto& action = world->AddComponent<UIActionComponent>(e);
		action.mState = UIActionState::Bounce;
		action.mActor = UIActor::Player;
		action.mIsLoop = true;
		action.mBounceAmplitude = kUIBeatBounceAmplitude;
	};

	for (int i = 0; i < 3; ++i)
	{
		const float cellX = kIconCol[i] * kCell;             // 아이콘 열 (Skill1/Skill2/궁극기)
		const float cellY = static_cast<float>(atlasRow) * kCell;
		const Vec2 skillKey = kKeyCell[i];                   // 키 텍스트 셀 (E/Shift/Q)

		const bool isUlt = (kSkillSlotId[i] == kUltimateSlot);
		const Vec2 slotSize = isUlt ? kUltimateSize : kSkillSize;
		const Vec2 backPadding = Vec2(kBackPadding, kBackPadding);
		const Vec2 backPosition = kSkillPos[i] - backPadding;
		const Vec2 backSize = slotSize + backPadding * 2.f;
#ifdef _IMGUI
		const std::string imguiSlotName = (i == 0) ? "Skill1" : (i == 1) ? "Skill2" : "Ultimate";
#endif


		Entity back = world->CreateEntity();
		{
			auto& t = world->AddComponent<UITransformComponent>(back);
			t.mAnchor = Anchor::BottomRight;
			t.mPosition = backPosition;
			t.mSize = backSize;
			t.mPivot = Vec2(0.f, 0.f);
			t.mUILayerIndex = 5;
			auto& sp = world->AddComponent<UISpriteComponent>(back, backTex);
			sp.SetSourceRect(0, 1024, 512, 512);
			sp.mVisible = true;
#ifdef _IMGUI
			props.push_back({ imguiSlotName + " Back Pos", PropertyType::Vec2, &(t.mPosition), 0.f, 0.f });
			props.push_back({ imguiSlotName + " Back Size", PropertyType::Vec2, &(t.mSize), 0.f, 0.f });
#endif
		}

		// 스킬 아이콘
		Entity icon = world->CreateEntity();
#ifdef _IMGUI
		if (i == 0)
			imguiOwner = icon;
#endif
		{
			auto& t = world->AddComponent<UITransformComponent>(icon);
			t.mAnchor = Anchor::BottomRight;
			t.mPosition = kSkillPos[i];
			t.mSize = slotSize;
			t.mUILayerIndex = 6;
			auto& sp = world->AddComponent<UISpriteComponent>(icon, skiilTex);
#ifdef _IMGUI
			props.push_back({ imguiSlotName + " Icon Pos", PropertyType::Vec2, &(t.mPosition), 0.f, 0.f });
			props.push_back({ imguiSlotName + " Icon Size", PropertyType::Vec2, &(t.mSize), 0.f, 0.f });
#endif
			sp.SetSourceRect(cellX, cellY, kCell, kCell);   // 아틀라스 아이콘 셀
			sp.mVisible = true;
		}

		// 쿨타임 오버레이
		Entity overlay = world->CreateEntity();
		{
			auto& t = world->AddComponent<UITransformComponent>(overlay);
			t.mAnchor = Anchor::BottomRight;
			t.mPosition = kSkillPos[i];
			t.mSize = slotSize;
			t.mPivot = Vec2(0.f, 0.f);
			t.mUILayerIndex = 7;
			auto& sp = world->AddComponent<UISpriteComponent>(overlay, overlayTex);
#ifdef _IMGUI
			props.push_back({ imguiSlotName + " Overlay Pos", PropertyType::Vec2, &(t.mPosition), 0.f, 0.f });
			props.push_back({ imguiSlotName + " Overlay Size", PropertyType::Vec2, &(t.mSize), 0.f, 0.f });
#endif
			sp.SetSourceRect(cellX, cellY, kCell, kCell);
			sp.mColorTint = Vec4(0.05f, 0.05f, 0.08f, 0.82f);  // 진한 반투명 덮개
			sp.mVisible = false;
		}


		// 키 패널
		Entity key = world->CreateEntity();
		{
			auto& t = world->AddComponent<UITransformComponent>(key);
			t.mAnchor = Anchor::BottomRight;
			t.mPosition = kSkillPos[i] + Vec2(0.f, 0.f);
			t.mSize = Vec2(72.f, 72.f);
			t.mUILayerIndex = 8;
			auto& sp = world->AddComponent<UISpriteComponent>(key, keyTex);
			sp.SetSourceRect(skillKey.x, skillKey.y, kCell, kCell);
			sp.mVisible = true;
#ifdef _IMGUI
			props.push_back({ imguiSlotName + " Key Pos", PropertyType::Vec2, &(t.mPosition), 0.f, 0.f });
			props.push_back({ imguiSlotName + " Key Size", PropertyType::Vec2, &(t.mSize), 0.f, 0.f });
#endif
		}


		addBeatBounce(back);
		addBeatBounce(icon);
		addBeatBounce(overlay);
		addBeatBounce(key);

		auto& slot = world->AddComponent<HUDSkillSlotComponent>(icon);
		slot.mSkillSlot = kSkillSlotId[i];
		slot.mKey = key;
		slot.mBack = back;
		slot.mIcon = icon;
		slot.mOverlay = overlay;
		slot.mIsUltimate = isUlt;
	}

#ifdef _IMGUI
	if (imguiOwner != NULL_ENTITY)
	{
		IMGUIComponent& imgui = world->AddComponent<IMGUIComponent>(imguiOwner);
		imgui.SetName("HUD Skill Bar Prefab");
		imgui.RegisterEditorProperties(props);
	}
#endif
}

HUDSkillBarPrefab::~HUDSkillBarPrefab()
{
}

HUDHPBarPrefab::HUDHPBarPrefab(World* world, uint8 playerType, Entity ownerEntity)
{
	{

#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			Entity back = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_HP_0");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_HP_0");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_HP_0");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(back);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(0.f, 576.f);
			t.mSize = Vec2(768.f, 256.f);   // 텍스처 원본 비율 3:1 (768x256)
			t.mPivot = Vec2(0.5f, 0.5f);
			t.mUILayerIndex = 2;

			world->AddComponent<UISpriteComponent>(back, scorem);
#ifdef _IMGUI



			props.push_back({ "Background HP Back Pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Background HP Back Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}

		{

			Entity hp = world->CreateEntity();

			shared_ptr<Texture> scorem;
			switch (playerType) {
			case 0:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Rudwig_HP_1");
				break;
			case 1:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ibanix_HP_1");
				break;
			case 2:
				scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Fanthor_HP_1");
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(hp);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(-384.f, 448.f);
			t.mSize = Vec2(768.f, 256.f);     
			t.mPivot = Vec2(0.0f, 0.0f);      
			t.mUILayerIndex = 1;


			world->AddComponent<UISpriteComponent>(hp, scorem);
			world->AddComponent<UIScriptComponent>(hp).mOnUpdate =
				[world, hp, ownerEntity](float /*dt*/)
			{
				HealthComponent* health = world->GetComponent<HealthComponent>(ownerEntity);
				if (health == nullptr || health->mMaxHp <= 0)
					return;
				UISpriteComponent* s = world->GetComponent<UISpriteComponent>(hp);
				if (s == nullptr)
					return;

				// 상용게임 오버실드 방식: 바 전체 스케일 denom = max(MaxHp, 현재체력+쉴드).
				//  - 체력+쉴드 <= MaxHp : denom=MaxHp 고정 → 체력 비율 유지, 쉴드는 잃은 체력 공백만 채움.
				//  - 체력+쉴드 >  MaxHp : 쉴드가 바를 넘쳐야 denom이 늘며 체력/쉴드 비율이 재조정됨.
				ArmorComponent* armor = world->GetComponent<ArmorComponent>(ownerEntity);
				const int32 shield = (armor != nullptr) ? (std::max)(0, armor->mCurrentArmor) : 0;
				const int32 curHp  = (std::max)(0, health->mCurrentHp);
				const float denom  = static_cast<float>((std::max)(health->mMaxHp, curHp + shield));

				const float ratio = std::clamp(
					static_cast<float>(curHp) / denom,
					0.0f, 1.0f);

				// 텍스처 여백 보정: 크롭 끝 위치를 캔버스 전체가 아닌
				// 실제 바 구간(UIHpBarComponent::mFillUvRangeX) 기준으로 리매핑
				Vec2 fillUvRange = Vec2(0.f, 1.f);
				if (UIHpBarComponent* barComp = world->GetComponent<UIHpBarComponent>(hp))
					fillUvRange = barComp->mFillUvRangeX;

				s->SetVisibleRangeKeepDestinationSize(false);
				s->SetVisibleRangeNormalizedX(0.f, RemapBarRatioToUv(ratio, fillUvRange));
			};

			// HP 바 자체는 위 UISprite 가 그리고, 여기서 부착하는 UIHpBarComponent 는
			// 파편 + hit effect 만 담당 (HUD 모드: 화면 픽셀 직접 좌표, depth 무시).
			{
				std::wstring hpBgName = L"UI_Fanthor_HP_0";
				std::wstring hpBarName = L"UI_Fanthor_HP_1";


				switch (playerType)
				{
				case PlayerType::Fanthor:
					hpBgName = L"UI_Fanthor_HP_0";
					hpBarName = L"UI_Fanthor_HP_1";
					break;
				case PlayerType::Rudwig:
					hpBgName = L"UI_Rudwig_HP_0";
					hpBarName = L"UI_Rudwig_HP_1";
					break;
				case PlayerType::Ibanix:
					hpBgName = L"UI_Ibanix_HP_0";
					hpBarName = L"UI_Ibanix_HP_1";
					break;
				}

				auto& bar = world->AddComponent<UIHpBarComponent>( hp, t.mSize.x, ownerEntity, Vec3::Zero, t.mSize.y, hpBgName, hpBarName);
				bar.mIsScreenSpace = true;
				bar.mRenderBgFill = false;            // 위 UISprite 가 이미 그림
				bar.mHitEffectTextureName = L"UI_Player_HP_3";
				bar.mHitEffectCols = 4;
				bar.mHitEffectRows = 1;
				bar.mHitEffectFrameCount = 4;
				bar.mHitEffectHeightScale = 2.0f;
				bar.mHitEffectOffsetPx = Vec2::Zero;

				// 텍스처 여백 보정: UI_<캐릭터>_HP_1 (768x256) 의 바 픽셀 영역
				// X 118~718 / Y 116~140 — 세 캐릭터 공통 (알파 스캔 측정, ±1px)
				bar.mFillUvRangeX = Vec2(118.f / 768.f, 718.f / 768.f);
				bar.mFillUvRangeY = Vec2(116.f / 256.f, 140.f / 256.f);
#ifdef _IMGUI
				props.push_back({ "Hud Hp Fill UV RangeX",  PropertyType::Vec2,  &(bar.mFillUvRangeX),   0.f,    0.f });
				props.push_back({ "Hud Hp Fill UV RangeY",  PropertyType::Vec2,  &(bar.mFillUvRangeY),   0.f,    0.f });
				props.push_back({ "Hud Hit Effect Scale",   PropertyType::Float, &(bar.mHitEffectHeightScale), 0.f, 8.f });
				props.push_back({ "Hud Hit Width Loss",     PropertyType::Float, &(bar.mHitEffectWidthLossScale), 0.f, 4.f });
#endif
			}
#ifdef _IMGUI

			props.push_back({ "Back Hp Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Back Hp Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif

		}
		{	// 쉴드(아머) 바 — 체력바 레이어 안에서 체력 오른쪽에 이어 붙여 표시
			Entity shield = world->CreateEntity();

			// 체력바 레이어(BACK 프레임 안의 실제 바 구간)의 디자인 좌표.
			// HP fill(pos -384,448 / size 768x256) + mFillUvRangeX(118~718/768) / Y(116~140/256) 에서 도출.
			const float barLeftX  = -266.f;   // -384 + 118
			const float barWidth  = 600.f;    // 718 - 118
			const float barTopY   = 564.f;    // 448 + 116
			const float barHeight = 24.f;     // 140 - 116

			auto& t = world->AddComponent<UITransformComponent>(shield);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(barLeftX, barTopY);   // 매 프레임 스크립트가 갱신
			t.mSize = Vec2(barWidth, barHeight);
			t.mPivot = Vec2(0.f, 0.f);
			t.mUILayerIndex = 1;                     // HP fill 과 동일 (BACK 프레임 아래)

			auto& sp = world->AddComponent<UISpriteComponent>(shield, RESOURCEMANAGER.Get<Texture>(L"UI_Player_Shield"));
			// 쉴드 텍스처(768x256)의 실제 바 픽셀 구간만 크롭 — HP 바 구간과 동일 좌표계로 정렬.
			sp.SetSourceRect(118.f, 116.f, 600.f, 24.f);
			sp.mVisible = false;

			world->AddComponent<UIScriptComponent>(shield).mOnUpdate =
				[world, shield, ownerEntity, barLeftX, barWidth, barTopY, barHeight](float /*dt*/)
			{
				UISpriteComponent* sp = world->GetComponent<UISpriteComponent>(shield);
				UITransformComponent* tr = world->GetComponent<UITransformComponent>(shield);
				if (sp == nullptr || tr == nullptr)
					return;

				HealthComponent* health = world->GetComponent<HealthComponent>(ownerEntity);
				ArmorComponent*  armor  = world->GetComponent<ArmorComponent>(ownerEntity);
				const int32 shieldAmt = (armor != nullptr) ? (std::max)(0, armor->mCurrentArmor) : 0;
				if (health == nullptr || health->mMaxHp <= 0 || shieldAmt <= 0)
				{
					sp->mVisible = false;   // 쉴드 없으면 숨김 → HP 바가 레이어 전체 사용
					return;
				}

				// 바 전체 스케일 denom = max(MaxHp, 현재체력+쉴드) (HP fill 과 동일 공식).
				// 쉴드는 "현재 체력" 바로 오른쪽 = [currentHp/denom, (currentHp+쉴드)/denom] 을 채운다.
				const int32 curHpI       = (std::max)(0, health->mCurrentHp);
				const float denom        = static_cast<float>((std::max)(health->mMaxHp, curHpI + shieldAmt));
				const float segLeftFrac  = static_cast<float>(curHpI)     / denom; // 현재 체력 끝 = 쉴드 시작
				const float segWidthFrac = static_cast<float>(shieldAmt)  / denom; // 쉴드 폭

				tr->mPosition = Vec2(barLeftX + barWidth * segLeftFrac, barTopY);
				tr->mSize     = Vec2(barWidth * segWidthFrac, barHeight);
				sp->mVisible  = true;
			};
		}
		{

			Entity hp = world->CreateEntity();
			auto& t = world->AddComponent<UITransformComponent>(hp);
			t.mAnchor = Anchor::Center;
			t.mPosition = Vec2(-256.f, 518.f);
			t.mSize = Vec2(256.f, 96.f);
			t.mPivot = Vec2(0.0f, 0.0f);
			t.mUILayerIndex = 5;

			auto& text = world->AddComponent<UITextComponent>(hp);
			text.mText = L"HP";
			text.mOnTextChanged = [world, hp, ownerEntity]() {
				HealthComponent* health = world->GetComponent<HealthComponent>(ownerEntity);
				if (health == nullptr) return;
				UITextComponent* t = world->GetComponent<UITextComponent>(hp);
				if (t)
					t->mText = std::to_wstring(health->mCurrentHp) + L"/" + std::to_wstring(health->mMaxHp);
			};

			
#ifdef _IMGUI

			props.push_back({ "Font Hp Position",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Font Hp Size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif

		



#ifdef _IMGUI


			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(hp);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("HP");
#endif
		}




	}
}

HUDHPBarPrefab::~HUDHPBarPrefab()
{

}

HUDBossHPBarPrefab::HUDBossHPBarPrefab(World* world, Entity bossEntity, int bossType)
{

	const std::wstring bossName = BossAssetName(bossType);
	const std::wstring backgroundTextureName = L"UI_Boss_" + bossName + L"_HP_0";
	const std::wstring fillTextureName = L"UI_Boss_" + bossName + L"_HP_1";

	const Vec2 bossBarSize = Vec2(1024.f, 160.f);
	const Vec2 bossBarPosition = Vec2(0.f, 32.f);
	const float fillStartX = 8.f / 1024.f;
	const float fillEndX = 1024.f / 1024.f;

	{
		Entity background = world->CreateEntity();
		auto& transform = world->AddComponent<UITransformComponent>(background);
		transform.mAnchor = Anchor::TopCenter;
		transform.mPosition = bossBarPosition;
		transform.mSize = bossBarSize;
		transform.mPivot = Vec2(0.5f, 0.f);
		transform.mUILayerIndex = 10;

		auto& sprite = world->AddComponent<UISpriteComponent>(
			background,
			RESOURCEMANAGER.Get<Texture>(backgroundTextureName));

		// 보스 엔티티가 제거되면 남아 있는 배경 HUD도 표시하지 않는다.
		world->AddComponent<UIScriptComponent>(background).mOnUpdate =
			[world, background, bossEntity](float)
			{
				UISpriteComponent* backgroundSprite = world->GetComponent<UISpriteComponent>(background);
				if (backgroundSprite == nullptr)
					return;

				backgroundSprite->mVisible = world->GetComponent<HealthComponent>(bossEntity) != nullptr;
			};
	}

	{
		Entity fill = world->CreateEntity();
		auto& transform = world->AddComponent<UITransformComponent>(fill);
		transform.mAnchor = Anchor::TopCenter;
		transform.mPosition = Vec2(-bossBarSize.x * 0.5f, bossBarPosition.y);
		transform.mSize = bossBarSize;
		transform.mPivot = Vec2(0.f, 0.f);
		transform.mUILayerIndex = 11;

		auto& sprite = world->AddComponent<UISpriteComponent>(
			fill,
			RESOURCEMANAGER.Get<Texture>(fillTextureName));
		sprite.SetVisibleRangeKeepDestinationSize(false);
		sprite.SetVisibleRangeNormalizedX(0.f, fillEndX);

		// 플레이어 로컬 HP HUD와 동일하게 UIHpBarComponent를 연결해 체력 손실 파편과 피격 효과를 생성한다.
		auto& hpBar = world->AddComponent<UIHpBarComponent>(
			fill,
			bossBarSize.x,
			bossEntity,
			Vec3::Zero,
			bossBarSize.y,
			backgroundTextureName,
			fillTextureName);
		hpBar.mIsScreenSpace = true;
		hpBar.mRenderBgFill = false;
		hpBar.mFillUvRangeX = Vec2(fillStartX, fillEndX);
		hpBar.mFillUvRangeY = Vec2(79.f / 160.f, 116.f / 160.f);
		hpBar.mHitEffectTextureName = L"UI_Player_HP_3";
		hpBar.mHitEffectCols = 4;
		hpBar.mHitEffectRows = 1;
		hpBar.mHitEffectFrameCount = 4;
		hpBar.mHitEffectHeightScale = 1.0f;
		hpBar.mHitEffectOffsetPx = Vec2::Zero;

		world->AddComponent<UIScriptComponent>(fill).mOnUpdate =
			[world, fill, bossEntity, fillStartX, fillEndX](float)
			{
				UISpriteComponent* fillSprite = world->GetComponent<UISpriteComponent>(fill);
				HealthComponent* health = world->GetComponent<HealthComponent>(bossEntity);
				if (fillSprite == nullptr)
					return;

				if (health == nullptr || health->mMaxHp <= 0)
				{
					fillSprite->mVisible = false;
					return;
				}

			
				const float hpRatio = std::clamp(
					static_cast<float>((std::max)(0, health->mCurrentHp)) /
					static_cast<float>(health->mMaxHp),
					0.f,
					1.f);
				const float visibleEndX = fillStartX + hpRatio * (fillEndX - fillStartX);

				fillSprite->mVisible = true;
				fillSprite->SetVisibleRangeKeepDestinationSize(false);
				fillSprite->SetVisibleRangeNormalizedX(0.f, visibleEndX);
			};
	}

}

HUDBossHPBarPrefab::~HUDBossHPBarPrefab()
{
}

HUDWeaponPrefab::HUDWeaponPrefab(World* world, uint8 playerType, Entity ownerEntity)
{
	// 캐릭터별 Display와 Weapon 이미지는 UI_Ingame_Sheet의 고정 셀을 사용한다.
	const float displaySourceX =
		playerType == PlayerType::Ibanix ? 768.0f :
		playerType == PlayerType::Rudwig ? 1024.0f : 1280.0f;
	const float weaponSourceX =
		playerType == PlayerType::Ibanix ? 0.0f :
		playerType == PlayerType::Rudwig ? 256.0f : 512.0f;
	const shared_ptr<Texture> ingameAtlas =
		RESOURCEMANAGER.Get<Texture>(L"UI_Ingame_Sheet");

	{
		const float  BounceAmplitude = 0.05f;
		const float  mBounceFrequency = 2.f;
		const float  mBounceDamping = 10.0f;


#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			Entity weapon = world->CreateEntity();

			auto& t = world->AddComponent<UITransformComponent>(weapon);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(352.f, -160.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			auto& sprite =
				world->AddComponent<UISpriteComponent>(weapon, ingameAtlas);
			sprite.SetSourceRect(displaySourceX, 2304.0f, 256.0f, 256.0f);
#ifdef _IMGUI



			props.push_back({ "Weaponback pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Weaponback size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// Bullet
			Entity bullet = world->CreateEntity();

			UITextComponent& text = world->AddComponent<UITextComponent>(bullet);
			text.mText = L"EMPTY";
			switch (playerType) {
			case Rudwig:
			{
				text.mText = L"";/*∞*/
				
			}
				break;
			case Ibanix:
			{
				text.mText = L"10/10";
				text.mOnTextChanged = [world, bullet, ownerEntity]() {
					MainPlayerComponent* p = world->GetComponent<MainPlayerComponent>(ownerEntity);
					if (p == nullptr) return;
					UITextComponent* t = world->GetComponent<UITextComponent>(bullet);
					if (t) t->mText = std::to_wstring(p->mNowBullet) + L"/" + std::to_wstring(p->mMaxBullet);
				};
			}
				break;
			case Fanthor:
			{
				text.mText = L"5/5";
				text.mOnTextChanged = [world, bullet, ownerEntity]() {
					MainPlayerComponent* p = world->GetComponent<MainPlayerComponent>(ownerEntity);
					if (p == nullptr) return;
					UITextComponent* t = world->GetComponent<UITextComponent>(bullet);
					if (t) t->mText = std::to_wstring(p->mNowBullet) + L"/" + std::to_wstring(p->mMaxBullet);
				};
			}
				break;
			}
			auto& t = world->AddComponent<UITransformComponent>(bullet);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(576.f, -160.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			//world->AddComponent<UISpriteComponent>(bullet, scorem);
			
			
#ifdef _IMGUI



			props.push_back({ "Bullet pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Bullet size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// Weapon
			Entity sound = world->CreateEntity();

			auto& t = world->AddComponent<UITransformComponent>(sound);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(432.f, -160.f);
			t.mSize = Vec2(128.f, 64.f);
			t.mUILayerIndex = 2;
			t.mPivot = Vec2(0.5f, 0.5f);

			auto& sprite =
				world->AddComponent<UISpriteComponent>(sound, ingameAtlas);
			sprite.SetSourceRect(weaponSourceX, 2368.0f, 256.0f, 128.0f);
#ifdef _IMGUI

			props.push_back({ "Weapon pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "Weapon size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif




			
#ifdef _IMGUI
		

			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(sound);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("Weapon");
#endif
		}




	}
}

HUDWeaponPrefab::~HUDWeaponPrefab()
{
}



HUDMusicPrefab::HUDMusicPrefab(World* world, uint8 playerType, Entity ownerEntity)
{
	// Music HUD의 배경도 Weapon HUD와 동일한 캐릭터별 Display 셀을 공유한다.
	const float displaySourceX =
		playerType == PlayerType::Ibanix ? 768.0f :
		playerType == PlayerType::Rudwig ? 1024.0f : 1280.0f;
	// 리듬 텍스트와 아이콘은 Fanthor, Ibanix, Rudwig 순서로 각 네 칸씩 배치되어 있다.
	const int32 rhythmAtlasBaseIndex =
		playerType == PlayerType::Fanthor ? 0 :
		playerType == PlayerType::Ibanix ? 4 : 8;
	const float rhythmPaintSourceX =
		playerType == PlayerType::Ibanix ? 1536.0f :
		playerType == PlayerType::Rudwig ? 1792.0f : 2048.0f;
	const shared_ptr<Texture> ingameAtlas =
		RESOURCEMANAGER.Get<Texture>(L"UI_Ingame_Sheet");

	{
		const float  BounceAmplitude = 0.05f;
		const float  mBounceFrequency = 2.f;
		const float  mBounceDamping = 10.0f;


#ifdef _IMGUI

		std::vector<EditorProperty> props;
#endif
		{	// BACK 0
			
			Entity back = world->CreateEntity();

			auto& t = world->AddComponent<UITransformComponent>(back);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(352.f, -96.f);
			t.mSize = Vec2(128.f, 96.f);
			t.mUILayerIndex = 2;
			t.mPivot = Vec2(0.5f, 0.5f);

			auto& sprite =
				world->AddComponent<UISpriteComponent>(back, ingameAtlas);
			sprite.SetSourceRect(displaySourceX, 2304.0f, 256.0f, 256.0f);
#ifdef _IMGUI



			props.push_back({ "MusicBack pos ",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "MusicBack size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });

#endif
		}
		{	// BACK 1
			Entity sound = world->CreateEntity();

			auto& t = world->AddComponent<UITransformComponent>(sound);
			t.mAnchor = Anchor::BottomLeft;
			t.mPosition = Vec2(480.f, -88.f);
			t.mSize = Vec2(192.f, 192.f);
			t.mUILayerIndex = 1;
			t.mPivot = Vec2(0.5f, 0.5f);
			world->AddComponent<UISpriteComponent>(sound, ingameAtlas);

			// 크로스헤어 위 중앙에는 물감 배경, 리듬 아이콘, 설명 텍스트를 순서대로 겹친다.
			auto createCenterRhythmSprite =
				[world, &ingameAtlas](
					const Vec2& position,
					const Vec2& size,
					uint8 layer,
					const RECT& sourceRect) -> Entity
				{
					Entity entity = world->CreateEntity();
					auto& transform =
						world->AddComponent<UITransformComponent>(entity);
					transform.mAnchor = Anchor::Center;
					transform.mPosition = position;
					transform.mSize = size;
					transform.mPivot = Vec2(0.5f, 0.5f);
					transform.mUILayerIndex = layer;

					auto& sprite =
						world->AddComponent<UISpriteComponent>(entity, ingameAtlas);
					sprite.SetSourceRect(
						static_cast<float>(sourceRect.left),
						static_cast<float>(sourceRect.top),
						static_cast<float>(sourceRect.right - sourceRect.left),
						static_cast<float>(sourceRect.bottom - sourceRect.top));
					return entity;
				};

			const Entity centerPaint = createCenterRhythmSprite(
				Vec2(0.0f, -260.0f),
				Vec2(220.0f, 220.0f),
				6,
				RECT{
					static_cast<LONG>(rhythmPaintSourceX),
					2304,
					static_cast<LONG>(rhythmPaintSourceX + 256.0f),
					2560 });
			const Entity centerIcon = createCenterRhythmSprite(
				Vec2(0.0f, -260.0f),
				Vec2(110.0f, 110.0f),
				7,
				RECT{ 0, 2560, 256, 2816 });
			const Entity centerText = createCenterRhythmSprite(
				Vec2(0.0f, -140.0f),
				Vec2(320.0f, 320.0f),
				8,
				RECT{ 0, 2816, 256, 3072 });

			// 현재 적용 중인 패시브 리듬을 항상 확인할 수 있도록
			// 크로스헤어 아래에 FadeOut되지 않는 소형 아이콘을 별도로 둔다.
			const Entity passiveRhythmIcon = createCenterRhythmSprite(
				Vec2(0.0f, 105.0f),
				Vec2(64.0f, 64.0f),
				6,
				RECT{ 0, 2560, 256, 2816 });

			// 중앙 리듬 알림은 선택이 바뀔 때만 나타나므로 처음에는 투명하게 시작한다.
			for (Entity entity : { centerPaint, centerIcon, centerText })
			{
				if (UISpriteComponent* sprite =
					world->GetComponent<UISpriteComponent>(entity))
				{
					sprite->mColorTint.w = 0.0f;
				}
			}

			const auto previousRhythm = std::make_shared<uint8>(0);
			const auto hasPreviousRhythm = std::make_shared<bool>(false);
			const auto centerNoticeElapsed = std::make_shared<float>(1.5f);

			UIScriptComponent& script = world->AddComponent<UIScriptComponent>(sound);
			script.mOnUpdate = [
				world,
				ownerEntity,
				sound,
				centerPaint,
				centerIcon,
				centerText,
				passiveRhythmIcon,
				rhythmAtlasBaseIndex,
				previousRhythm,
				hasPreviousRhythm,
				centerNoticeElapsed](float deltaTime) {

				UISpriteComponent* sprite = world->GetComponent<UISpriteComponent>(sound);
				if (!sprite)
					return;

				MainPlayerComponent* player =
					world->GetComponent<MainPlayerComponent>(ownerEntity);
				if (!player)
					return;

				uint8 rhythmIndex = player->mRhythm;
				if (player->mDesiredRhythm != player->mRhythm)
					rhythmIndex = player->mDesiredRhythm;
				if (rhythmIndex >= 4)
					rhythmIndex = 0;
				const uint8 currentRhythmIndex =
					player->mRhythm < 4 ? player->mRhythm : 0;

				// 첫 갱신에서는 현재 상태만 기억하고 알림을 표시하지 않는다.
				// 이후 우클릭으로 선택 리듬이 바뀔 때마다 표시 시간을 처음부터 다시 시작한다.
				if (!*hasPreviousRhythm)
				{
					*previousRhythm = rhythmIndex;
					*hasPreviousRhythm = true;
				}
				else if (*previousRhythm != rhythmIndex)
				{
					*previousRhythm = rhythmIndex;
					*centerNoticeElapsed = 0.0f;
				}

				const float sourceX = static_cast<float>(
					(rhythmAtlasBaseIndex + rhythmIndex) * 256);

				// 좌하단과 중앙 텍스트는 같은 아틀라스 셀을 사용한다.
				sprite->SetSourceRect(sourceX, 2816.0f, 256.0f, 256.0f);

				if (UISpriteComponent* iconSprite =
					world->GetComponent<UISpriteComponent>(centerIcon))
				{
					iconSprite->SetSourceRect(
						sourceX, 2560.0f, 256.0f, 256.0f);
				}

				if (UISpriteComponent* textSprite =
					world->GetComponent<UISpriteComponent>(centerText))
				{
					textSprite->SetSourceRect(
						sourceX, 2816.0f, 256.0f, 256.0f);
				}

				// 패시브 아이콘은 FadeOut 대상이 아니며 현재 선택 리듬을 계속 표시한다.
				if (UISpriteComponent* passiveSprite =
					world->GetComponent<UISpriteComponent>(passiveRhythmIcon))
				{
					const float passiveSourceX = static_cast<float>(
						(rhythmAtlasBaseIndex + currentRhythmIndex) * 256);
					passiveSprite->SetSourceRect(
						passiveSourceX, 2560.0f, 256.0f, 256.0f);
					passiveSprite->mColorTint.w = 1.0f;
				}

				// 선택 직후 0.7초 동안 완전히 표시하고 다음 1.0초 동안 서서히 사라진다.
				constexpr float holdDuration = 0.7f;
				constexpr float fadeDuration = 1.f;
				constexpr float totalDuration = holdDuration + fadeDuration;

				float alpha = 0.0f;
				if (*centerNoticeElapsed < holdDuration)
				{
					alpha = 1.0f;
				}
				else if (*centerNoticeElapsed < totalDuration)
				{
					alpha = 1.0f -
						(*centerNoticeElapsed - holdDuration) / fadeDuration;
				}

				for (Entity entity : { centerPaint, centerIcon, centerText })
				{
					if (UISpriteComponent* centerSprite =
						world->GetComponent<UISpriteComponent>(entity))
					{
						centerSprite->mColorTint.w =
							std::clamp(alpha, 0.0f, 1.0f);
					}
				}

				*centerNoticeElapsed =
					(std::min)(*centerNoticeElapsed + deltaTime, totalDuration);
			};
			



#ifdef _IMGUI

			props.push_back({ "MusicName pos",  PropertyType::Vec2,  &(t.mPosition),   0.f,    0.f });
			props.push_back({ "MusicName size",  PropertyType::Vec2,  &(t.mSize),   0.f,    0.f });
			if (UITransformComponent* passiveTransform =
				world->GetComponent<UITransformComponent>(passiveRhythmIcon))
			{
				// HUDMusicPrefab 디버그 패널에서 패시브 리듬 아이콘의 위치와 크기를 조절한다.
				props.push_back({
					"PassiveRhythmIcon pos",
					PropertyType::Vec2,
					&(passiveTransform->mPosition),
					0.f,
					0.f });
				props.push_back({
					"PassiveRhythmIcon size",
					PropertyType::Vec2,
					&(passiveTransform->mSize),
					0.f,
					0.f });
			}

#endif





#ifdef _IMGUI


			IMGUIComponent& visImgui = world->AddComponent<IMGUIComponent>(sound);
			visImgui.RegisterEditorProperties(props);
			visImgui.SetName("Music");
#endif
		}




	}

	//{	// BACK 0

	//	Entity Bot = world->CreateEntity();

	//	shared_ptr<Texture> scorem = RESOURCEMANAGER.Get<Texture>(L"UI_Ingame_Back");
	//
	//	auto& t = world->AddComponent<UITransformComponent>(Bot);
	//	t.mAnchor = Anchor::Center;
	//	t.mSize = Vec2(2560.f, 1440.f);
	//	t.mUILayerIndex = 2;
	//	t.mPivot = Vec2(0.5f, 0.5f);

	//	world->AddComponent<UISpriteComponent>(Bot, scorem);
	//}

}

HUDMusicPrefab::~HUDMusicPrefab()
{

}



HUDCrosshairPrefab::HUDCrosshairPrefab(World* world)
{
	// 리듬바 전체를 중앙으로 당기고 기존 크기의 절반으로 축소한다.
	constexpr float rhythmTargetDistance = 70.f;
	constexpr float rhythmSpawnDistance = 460.f;
	constexpr float rhythmTargetSize = 59.f;
	constexpr float rhythmNoteSize = 56.f;

	const shared_ptr<Texture> leftRhythmTexture =
		RESOURCEMANAGER.Get<Texture>(L"UI_Aim_Bar_Left");
	const shared_ptr<Texture> rightRhythmTexture =
		RESOURCEMANAGER.Get<Texture>(L"UI_Aim_Bar_Right");

	auto createRhythmBar =
		[world](const shared_ptr<Texture>& texture, const Vec2& position, float size, uint8 layer, float alpha)
		{
			Entity entity = world->CreateEntity();

			auto& transform = world->AddComponent<UITransformComponent>(entity);
			transform.mAnchor = Anchor::Center;
			transform.mPosition = position;
			transform.mSize = Vec2(size, size);
			transform.mUILayerIndex = layer;
			transform.mPivot = Vec2(0.5f, 0.5f);

			auto& sprite = world->AddComponent<UISpriteComponent>(entity, texture);
			sprite.mColorTint.w = alpha;

			return entity;
		};

	// 중앙의 큰 좌우 표식은 박자가 도착해야 하는 고정 판정선으로 사용한다.
	const Entity leftTarget = createRhythmBar(
		leftRhythmTexture,
		Vec2(-rhythmTargetDistance, 0.f),
		rhythmTargetSize,
		4,
		0.75f);
	const Entity rightTarget = createRhythmBar(
		rightRhythmTexture,
		Vec2(rhythmTargetDistance, 0.f),
		rhythmTargetSize,
		4,
		0.75f);

	// 좌우 이동 표식은 같은 박자 진행률을 사용하고 잔상만 바깥쪽에 떨어뜨려 표시한다.
	const Entity leftNote = createRhythmBar(
		leftRhythmTexture,
		Vec2(-rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		5,
		1.f);
	const Entity rightNote = createRhythmBar(
		rightRhythmTexture,
		Vec2(rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		5,
		1.f);
	const Entity leftTrailNear = createRhythmBar(
		leftRhythmTexture,
		Vec2(-rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		3,
		1.f);
	const Entity rightTrailNear = createRhythmBar(
		rightRhythmTexture,
		Vec2(rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		3,
		1.f);
	const Entity leftTrailFar = createRhythmBar(
		leftRhythmTexture,
		Vec2(-rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		3,
		1.f);
	const Entity rightTrailFar = createRhythmBar(
		rightRhythmTexture,
		Vec2(rhythmSpawnDistance, 0.f),
		rhythmNoteSize,
		3,
		1.f);

	Entity rhythmController = world->CreateEntity();
	world->AddComponent<UIScriptComponent>(rhythmController).mOnUpdate =
		[world,
		 leftTarget,
		 rightTarget,
		 leftNote,
		 rightNote,
		 leftTrailNear,
		 rightTrailNear,
		 leftTrailFar,
		 rightTrailFar,
		 rhythmTargetDistance,
		 rhythmSpawnDistance](float)
		{
			BeatSystem* beatSystem = nullptr;
			if (auto systemManager = world->GetSystemManager())
				beatSystem = systemManager->GetSystem<BeatSystem>();

			if (beatSystem == nullptr || beatSystem->GetBeatSeconds() <= 0.f)
				return;

			const float beatSeconds = beatSystem->GetBeatSeconds();
			const float beatProgress =
				std::fmod(beatSystem->GetSongPosition(), beatSeconds) / beatSeconds;
			const float noteSpacing =
				(rhythmSpawnDistance - rhythmTargetDistance) / 3.f;

			auto updateRhythmNote =
				[world, rhythmTargetDistance, rhythmSpawnDistance](Entity entity, float signedDistance)
				{
					UITransformComponent* transform =
						world->GetComponent<UITransformComponent>(entity);
					UISpriteComponent* sprite =
						world->GetComponent<UISpriteComponent>(entity);
					if (transform == nullptr || sprite == nullptr)
						return;

					const float distance = std::abs(signedDistance);
					const float distanceRatio = std::clamp(
						(distance - rhythmTargetDistance) /
						(rhythmSpawnDistance - rhythmTargetDistance),
						0.f,
						1.f);

					transform->mPosition.x = signedDistance;

					// 바깥쪽 표식일수록 작고 흐리게 표시해 앞 표식과 뒤 표식의 깊이를 구분한다.
					const float noteScale = std::lerp(1.f, 0.68f, distanceRatio);
					transform->mScale = Vec2(noteScale, noteScale);
					sprite->mColorTint.w = std::lerp(1.f, 0.14f, distanceRatio);
				};

			// 세 표식을 고정 간격의 대열로 이동시킨다.
			// 박자 경계에서는 앞 표식 하나만 사라지고 동일한 표식이 대열 맨 뒤로 순환한다.
			const float firstDistance =
				rhythmTargetDistance + noteSpacing * (1.f - beatProgress);
			const float secondDistance = firstDistance + noteSpacing;
			const float thirdDistance = secondDistance + noteSpacing;

			updateRhythmNote(leftNote, -firstDistance);
			updateRhythmNote(rightNote, firstDistance);
			updateRhythmNote(leftTrailNear, -secondDistance);
			updateRhythmNote(rightTrailNear, secondDistance);
			updateRhythmNote(leftTrailFar, -thirdDistance);
			updateRhythmNote(rightTrailFar, thirdDistance);

			// 박자 경계에서 고정 판정선을 짧게 확대해 표식 도착 시점을 명확하게 보여 준다.
			const float distanceFromBeat = (std::min)(beatProgress, 1.f - beatProgress);
			const float pulseRatio = std::clamp(1.f - distanceFromBeat / 0.16f, 0.f, 1.f);
			const float targetScale = 1.f + pulseRatio * 0.22f;

			if (UITransformComponent* transform =
				world->GetComponent<UITransformComponent>(leftTarget))
			{
				transform->mScale = Vec2(targetScale, targetScale);
			}
			if (UITransformComponent* transform =
				world->GetComponent<UITransformComponent>(rightTarget))
			{
				transform->mScale = Vec2(targetScale, targetScale);
			}

		};

	// 기본 크로스헤어 (항상 표시)
	Entity crosshair = world->CreateEntity();
	{
		shared_ptr<Texture> scorem = RESOURCEMANAGER.Get<Texture>(L"jAims");
		auto& t = world->AddComponent<UITransformComponent>(crosshair);
		t.mAnchor = Anchor::Center;
		t.mPosition = Vec2(0.f, 0.f);
		t.mSize = Vec2(128.f, 128.f);
		t.mUILayerIndex = 5;
		t.mPivot = Vec2(0.5f, 0.5f);
		world->AddComponent<UISpriteComponent>(crosshair, scorem);
	}

}

HUDCrosshairPrefab::~HUDCrosshairPrefab()
{
}

HUDScorePrefab::HUDScorePrefab(World* world)
{


}

HUDScorePrefab::~HUDScorePrefab()
{
}

HUDTimerPrefab::HUDTimerPrefab(World* world)
{
	Entity timer = world->CreateEntity();
	auto& dbgTransform = world->AddComponent<UITransformComponent>(timer);
	dbgTransform.mAnchor = Anchor::Center; // 화면 좌측 상단
	dbgTransform.mPosition = Vec2(30.f, -30.f);
	dbgTransform.mSize = Vec2(400.f, 50.f);
	dbgTransform.mUILayerIndex = 15;

	world->AddComponent<UIScriptComponent>(timer).mOnUpdate =
		[world, timer](float /*dt*/)
		{
			GameRuleComponent* gameRuleComp = world->GetSingleton<GameRuleComponent>();
			UITextComponent* textComp = world->GetComponent<UITextComponent>(timer);

			if (gameRuleComp && textComp)
			{
				// x, y, z 좌표를 읽어와 텍스트 갱신
				std::wstring timeText = std::to_wstring(gameRuleComp->mGameTime);

				textComp->mText = timeText;
			}
		};
}

HUDTimerPrefab::~HUDTimerPrefab()
{
}

AreaConquestPrefab::AreaConquestPrefab(World* world)
{
	InputCommand ctx; // 기본값으로 빈 InputCommand 생성
	PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Conquer", Vec3(50.f, 50.f, 50.f));
}

AreaConquestPrefab::~AreaConquestPrefab()
{
}

Entity AreaConquestPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Conquer", Vec3(50.f, 50.f, 50.f));
}

AreaEscortPrefab::AreaEscortPrefab(World* world)
{
	InputCommand ctx; // 기본값으로 빈 InputCommand 생성
	PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Escort", Vec3(50.f, 50.f, 50.f));

}

AreaEscortPrefab::~AreaEscortPrefab()
{
}


Entity AreaEscortPrefab::Build(World* world, const InputCommand& ctx)
{
	return PrefabFactory::BuildWorldMarkerPrefab(world, ctx, L"VFX_Sector_Escort", Vec3(50.f, 50.f, 50.f));
}


TruckEscortPrefab::TruckEscortPrefab(World* world)
{
	Entity truck = world->CreateEntity();
	world->AddComponent<TransformComponent>(truck);

	shared_ptr<Mesh> phereMesh;
	std::vector<shared_ptr<Material>> material2s;
	shared_ptr<Material> material2;


	phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Escort");
	material2 = RESOURCEMANAGER.Get<Material>(L"SM_Escort0");


	material2s.push_back(material2);
	world->AddComponent<NetEntityComponent>(truck);
	world->AddComponent<NetTransformComponent>(truck);
	world->AddComponent<RenderComponent>(truck, phereMesh, material2s);

}

TruckEscortPrefab::~TruckEscortPrefab()
{
}

// 트럭을 따라다니는 부착형 VFX를 트럭의 자식 엔티티로 생성
static Entity AttachTruckVfx(World* world, Entity truck, const wstring& vfxName, const Vec3& offset, const Vec3& scale)
{
	Entity child = world->CreateEntity();
	world->AddComponent<TransformComponent>(child);

	// TransformComponent 풀이 재할당될 수 있으므로 추가 이후 다시 조회한다.
	TransformComponent* childTrans = world->GetComponent<TransformComponent>(child);
	TransformComponent* truckTrans = world->GetComponent<TransformComponent>(truck);
	childTrans->mParent = truck;
	childTrans->mIsStatic = true;            // 부모(트럭)가 transform을 구동
	truckTrans->mChild.push_back(child);

	VfxComponent& vfx = world->AddComponent<VfxComponent>(child);
	vfx.mVfx = RESOURCEMANAGER.Get<Vfx>(vfxName);
	vfx.mScale = scale;
	vfx.mAttachOffset = offset;              // 위치는 호출부 테이블에서 조정
	vfx.mIsLoop = true;
	vfx.mRestartWhenFinished = false;
	vfx.mShouldPlay = false;                 // 이동 중에만 재생 (GamePhaseSystem이 토글)
	vfx.mIsPooled = false;
	vfx.mInUse = false;
	vfx.mAutoReturn = false;
	return child;
}

Entity TruckEscortPrefab::Build(World* world, const InputCommand& ctx)
{
	Entity truck = world->CreateEntity();
	TransformComponent& trans = world->AddComponent<TransformComponent>(truck);
	trans.mLocalScale = Vec3(100.f, 100.f, 100.f);

	shared_ptr<Mesh> phereMesh;
	std::vector<shared_ptr<Material>> material2s;
	shared_ptr<Material> material2;

	
	phereMesh = RESOURCEMANAGER.Get<Mesh>(L"SM_Escort_Body");
	material2 = RESOURCEMANAGER.Get<Material>(L"SM_Escort0");


	material2s.push_back(material2);
	auto& netComp = world->AddComponent<NetEntityComponent>(truck);
	netComp.mOwnerEntity = truck;
	netComp.mNetEntityId = ctx.ViewAs<S2C_SpawnPacekt>()->netEntityId;
	world->NetIdBinding(netComp.mNetEntityId, truck);
	world->AddComponent<NetTransformComponent>(truck);
	world->AddComponent<RenderComponent>(truck, phereMesh, material2s);

	// 트럭을 따라다니는 부착형 VFX 목록. { 이펙트 이름, 부착 오프셋, 스케일 }
	struct TruckVfxDesc { const wchar_t* name; Vec3 offset; Vec3 scale; };
	static const TruckVfxDesc truckVfxList[] =
	{
		{ L"VFX_Escort_Shockwave", Vec3(0.f, 230.f, 175.f), Vec3(15.f, 15.f, 15.f) },
		// Pulse 4개 — 위치(offset)만 서로 다르게 조정하면 됨
		{ L"VFX_Escort_Pulse",     Vec3(-130.f, 0.f, -110.f), Vec3(5.f, 5.f, 5.f) },
		{ L"VFX_Escort_Pulse",     Vec3(130.f, 0.f, -110.f), Vec3(5.f, 5.f, 5.f) },
		{ L"VFX_Escort_Pulse",     Vec3(-130.f, 0.f, 200.f), Vec3(5.f,5.f, 5.f) },
		{ L"VFX_Escort_Pulse",     Vec3(130.f, 0.f, 200.f), Vec3(5.f, 5.f, 5.f) },
	};
	for (const TruckVfxDesc& desc : truckVfxList)
		AttachTruckVfx(world, truck, desc.name, desc.offset, desc.scale);

	// 호위 범위 링
	{
		constexpr float kEscortRange = 1000.f;
		Entity ring = DecalFactory::SpawnRing(world, trans.mLocalPosition, kEscortRange,
			Vec4(0.2f, 1.6f, 2.4f, 0.8f), 25.f, -1.f);
		DecalComponent& rd = *world->GetComponent<DecalComponent>(ring);
		rd.FollowTarget      = truck;
		rd.DestroyWithTarget = true;
		rd.Height            = 150.f;
		rd.NormalThreshold   = 0.35f;

		auto noteTex = RESOURCEMANAGER.Get<Texture>(L"DecalNote");
		rd.TexIndex = noteTex ? static_cast<int>(noteTex->GetImageIndex()) : -1;
	}

	return truck;
}
