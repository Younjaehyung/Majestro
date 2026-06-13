#include "pch.h"

#include "RenderSystem.h"
#include "AnimationComponent.h"
#include "BulletComponent.h"
#include "BoxColliderComponent.h"
#include "CameraComponent.h"
#include "Engine.h"
#include "Material.h"
#include "ParticleComponent.h"
#include "RenderComponent.h"
#include "RenderManager.h"
#include "RenderTarget.h"
#include "ResourceManager.h"
#include "TagComponent.h"
#include "TerrainComponent.h"
#include "World.h"
#include "Timer.h"

#include "PlayerComponent.h"
#include "EffectFlagComponent.h"
#include "GameRenderPipeline.h"
#include "DamageFeedbackComponent.h"
#include "RhythmEmissiveComponent.h"
// 정적 멤버 정의
std::vector<DebugLineRequest> RenderSystem::sDebugLineQueue;
bool RenderSystem::sDrawColliders = true;
bool RenderSystem::sDrawCullingOBB = false;
bool RenderSystem::sDrawEnemyRanges = true;
bool RenderSystem::sDrawEnemyAttackRanges = true;
bool RenderSystem::sDrawPlayerAttackRanges = true;

void RenderSystem::SubmitDebugLine(const Vec3& start, const Vec3& end, const Vec4& color)
{
  sDebugLineQueue.push_back({ start, end, color });
}

void RenderSystem::SetDrawColliders(bool enabled)
{
  sDrawColliders = enabled;
}

bool RenderSystem::GetDrawColliders()
{
  return sDrawColliders;
}

void RenderSystem::SetDrawCullingOBB(bool enabled)
{
  sDrawCullingOBB = enabled;
}

bool RenderSystem::GetDrawCullingOBB()
{
  return sDrawCullingOBB;
}

void RenderSystem::SetDrawEnemyRanges(bool enabled)
{
  sDrawEnemyRanges = enabled;
}

bool RenderSystem::GetDrawEnemyRanges()
{
  return sDrawEnemyRanges;
}

void RenderSystem::SetDrawEnemyAttackRanges(bool enabled)
{
  sDrawEnemyAttackRanges = enabled;
}

bool RenderSystem::GetDrawEnemyAttackRanges()
{
  return sDrawEnemyAttackRanges;
}

void RenderSystem::SetDrawPlayerAttackRanges(bool enabled)
{
  sDrawPlayerAttackRanges = enabled;
}

bool RenderSystem::GetDrawPlayerAttackRanges()
{
  return sDrawPlayerAttackRanges;
}

RenderSystem::RenderSystem(World *world) : System::System(world) {
  mCamera = nullptr;
  mPhase = SysPhase::Render;
  mOrder = 0;
}

void RenderSystem::Initialize() {
  mRenderComponentPool = &(mWorld->GetComponentPool<RenderComponent>());
  mRootSignature = RESOURCEMANAGER.Get<RootSignature>(L"MainRootSignature");

  // immutability Data
  PushMaterialData();

  mWireCube = RESOURCEMANAGER.Get<Mesh>(L"WireCube");
  mLineMesh  = RESOURCEMANAGER.Get<Mesh>(L"Line");
  mDebugLineMat = RESOURCEMANAGER.Get<Material>(L"DebugLine");
  mDebugLineNoDepthMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_NoDepth");

  mDebugLineGreenMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_Green");
  mDebugLineRedMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_Red");
  mDebugLineYellowMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_Yellow");

  mDeferredDrawItems.reserve(1000);
  mDeferredDrawBatchs.reserve(1000);
  mInstanceVector.reserve(1000);

}

void RenderSystem::Update() {
  if (false == mWorld->HasComponentPool<MainCameraComponent>())
    return;

  auto cameraView = mWorld->View<MainCameraComponent>();
  auto cameraIt = cameraView.begin();
  if (cameraIt == cameraView.end()) {
      return;
  }
  mCameraID = *cameraIt;
  mCamera = mWorld->GetComponent<CameraComponent>(mCameraID);

  if (!mActivePipeline) return;

  ClearRTV();
  PushData();

  // 컨텍스트 구성 — 파이프라인에 렌더 데이터 전달
  RenderContext ctx;
  ctx.deferredBatchs  = &mDeferredDrawBatchs;
  ctx.cascadeBatchs   = &mCascadeDrawBatchs;
  ctx.lightBatchs     = &mLightDrawBatchs;
  ctx.cascadeActive  = &mCascadeActive;
  ctx.mapCascadeDirty = &mMapCascadeDirty;
  ctx.camera         = mCamera;
  ctx.deltaTime      = DELTA_TIME;
  ctx.frameIndex     = mFrameCount;

  mActivePipeline->PreCompute(ctx);
  mActivePipeline->Execute(ctx);
}

void RenderSystem::PushData() {
  RENDERMANAGER.SetGraphicsTable();
  // PushDebugging();
  PushLandData();
  PushCubeData();

  PushShadowCascades(); // 라이트 구체 사전 계산 
  PushObjectData();
  PushLightData();
  PushPassData();
  PushFrameData();
  PushInstanceData();
}

void RenderSystem::SetPipeline(shared_ptr<IRenderPipeline> pipeline)
{
    pipeline->Initialize(mWorld);
    mActivePipeline = pipeline;

}

void RenderSystem::ClearRTV() {

    // HDR Group 초기화
    auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
    hdrGroup.ClearRenderTargetView();

  // SwapChain Group 초기화
  uint8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();
  // RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).ClearRenderTargetView(backIndex);

  if (RENDERMANAGER.IsMsaaEnabled()) // msaa
  {
    auto &finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN));
    finalGroup.WaitResourceToTarget();
    finalGroup.ClearRenderTargetView(backIndex);
    finalGroup.WaitTargetToResource();
  }else {
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).ClearRenderTargetView(backIndex);
  }

  // Shadow Group 초기화
 // RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).ClearRenderTargetView();

  // Deferred Group 초기화
  RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).ClearRenderTargetView();

  // Lighting Group 초기화
  RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).ClearRenderTargetView();

 
  //// POST_HDR Group 초기화
  //RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::POST_HDR_A)).ClearRenderTargetView();
  //RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::POST_HDR_B)).ClearRenderTargetView();

  //// POST_LDR Group 초기화
  //RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::POST_LDR_A)).ClearRenderTargetView();
  //RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::POST_LDR_B)).ClearRenderTargetView();

  mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

  ClearBuffer();
  // 추후 lightvector관련들 clear도 모두 확인할껏.
}

void RenderSystem::ClearBuffer() {
  mCurrPSOID = 0;

  mDeferredDrawItems.clear();
  for (auto& items   : mCascadeDrawItems)  items.clear();
  for (auto& batches : mCascadeDrawBatchs) batches.clear();
  mDeferredDrawBatchs.clear();
  mLightDrawBatchs.clear();
  mInstanceVector.clear();
  // passParams = {};

  mLightVector.clear();
  mObjectVector.clear();
  mDummyVector.clear();
}

void RenderSystem::PushFrameData() {



  passParams.MatPrevView = passParams.MatView;
  passParams.MatPrevProjection = passParams.MatProjection;
  passParams.MatPrevViewInv = passParams.MatViewInv;
  passParams.MatPrevProjectionInv   = passParams.MatProjectionInv;

  passParams.MatView            = mCamera->mView.Transpose();
  passParams.MatProjection      = mCamera->mProjection.Transpose();
  passParams.MatViewInv         = mCamera->mView.Invert().Transpose();
  passParams.MatProjectionInv   = mCamera->mProjection.Invert().Transpose();

  passParams.ScreenSize = {
      static_cast<float>(RENDERMANAGER.GetWindow().Width),
      static_cast<float>(RENDERMANAGER.GetWindow().Height)};
  passParams.TotalTime = TIMER.GetTotalTime();
  // .z(=마지막 split)와 .w(하드 컷 거리)는 카메라 far — 맵 고정 cascade가 그 너머까지 커버
  if (mHasDirectionalShadow && mCamera)
      passParams.CascadeSplitDistances =
      Vec4(CascadeSplit[0], CascadeSplit[1], CascadeSplit[2], mCamera->mFar);
  else
      passParams.CascadeSplitDistances = Vec4(0.f, 0.f, 0.f, 0.f);

  // UpdateCascadeShadowMatrices는 PushShadowCascades()에서 이미 처리됨

  shared_ptr<GroupBuffer> groupBuffer =RENDERMANAGER.GetGroupBuffer(mFrameCount);

  groupBuffer->PassInfo->PushData(&passParams, sizeof(PassParams));
}

void RenderSystem::PushDebugging()
{
    //PushObjectData()가 sDebugLineQueue를 소비하므로
    //PushData() 이전에 디버그 라인을 먼저 큐에 추가해야 함
    {
        auto navMesh = RESOURCEMANAGER.Get<NavMesh>(L"NavMesh");
        if (navMesh && navMesh->mDtNavMesh)
            mNavMeshDebugRenderer.RenderNavMesh(navMesh->mDtNavMesh);
    }
}

void RenderSystem::PushMaterialData() {

  uint32 index{};
  for (auto &materials : RESOURCEMANAGER.GetAllResources<Material>()) {
    shared_ptr<Material> material =
        dynamic_pointer_cast<Material>(materials.second);
    material->SetIndex(index);
    mMaterialVector.emplace_back(material->GetParams());
    index++;
  }
  RENDERMANAGER.GetMaterialBuffers()->PushDefaultToData(
      mMaterialVector.data(),
      static_cast<uint32>(mMaterialVector.size() * sizeof(MaterialParams)));
}

void RenderSystem::PushCubeData() {
  // 스카이박스
  passParams.SkyBoxIndex = 0;
  shared_ptr<Material> skyboxMaterial =
      RESOURCEMANAGER.Get<Material>(L"Skybox");
  if (skyboxMaterial) {
    shared_ptr<Texture> skyboxTexture =
        skyboxMaterial->GetTexture(DIFFUSEMAP0INDEX);
    if (skyboxTexture && skyboxTexture->IsCubeMap())
      passParams.SkyBoxIndex = skyboxTexture->GetCubeMapIndex();
  }

  // IBL 큐브맵 인덱스
  passParams.IrradianceIndex     = -1;
  passParams.PreFilteredEnvIndex = -1;
  passParams.BrdfLutIndex        = -1;
  passParams.PreFilteredMipLevels = 8;

  auto irradianceTex = RESOURCEMANAGER.Get<Texture>(L"IBL_Irradiance");
  if (irradianceTex && irradianceTex->IsCubeMap())
    passParams.IrradianceIndex = static_cast<int32>(irradianceTex->GetCubeMapIndex());

  auto preFilteredTex = RESOURCEMANAGER.Get<Texture>(L"IBL_PreFiltered");
  if (preFilteredTex && preFilteredTex->IsCubeMap())
    passParams.PreFilteredEnvIndex = static_cast<int32>(preFilteredTex->GetCubeMapIndex());

  // BRDF LUT는 2D 텍스처 → TextureMaps 배열 인덱스
  auto brdfLutTex = RESOURCEMANAGER.Get<Texture>(L"IBL_BrdfLut");
  if (brdfLutTex)
    passParams.BrdfLutIndex = static_cast<int32>(brdfLutTex->GetImageIndex());
}

void RenderSystem::PushLandData() {
  passParams.HeightMapResolution = Vec2(0.f, 0.f);
  passParams.MaxTessLevel = 0.f;
  passParams.MinMaxTessDistance = Vec2(0.f, 0.f);
  passParams.TileCountX = 0;
  passParams.TileCountZ = 0;

  passParams.TerrainSlot1 = -1;
  passParams.TerrainSlot2 = -1;
  passParams.TerrainSlot3 = -1;
  passParams.TerrainSlot4 = -1;
  passParams.TerrainSlot5 = -1;
  passParams.TerrainSlot6 = -1;

  if (false == mWorld->HasComponentPool<TerrainComponent>())
      return;
  auto entity = mWorld->GetEntitiesWithComponent<TerrainComponent>();
  if (entity.empty())
      return;

  auto* terrainComp = mWorld->GetComponent<TerrainComponent>(entity[0]);
  if (terrainComp == nullptr)
      return;

  auto terrain = terrainComp->mTerrainParams;

  passParams.HeightMapResolution = terrain.HeightMapResolution;
  passParams.MaxTessLevel = terrain.MaxTessLevel;
  passParams.MinMaxTessDistance = terrain.MinMaxTessDistance;
  passParams.TileCountX = terrain.TileCountX;
  passParams.TileCountZ = terrain.TileCountZ;

  auto* recomp = mWorld->GetComponent<RenderComponent>(entity[0]);
  if (recomp == nullptr)
      return;


  switch (recomp->mMaterials.size()) {
  case 6:
    if (recomp->mMaterials[5] && recomp->mMaterials[5]->GetID())
      passParams.TerrainSlot6 = recomp->mMaterials[5]->GetIndex();
    //
  case 5:
    if (recomp->mMaterials[4] && recomp->mMaterials[4]->GetID())
      passParams.TerrainSlot5 = recomp->mMaterials[4]->GetIndex();
    //
  case 4:
    if (recomp->mMaterials[3] && recomp->mMaterials[3]->GetID())
      passParams.TerrainSlot4 = recomp->mMaterials[3]->GetIndex();
    //
  case 3:
    if (recomp->mMaterials[2] && recomp->mMaterials[2]->GetID())
      passParams.TerrainSlot3 = recomp->mMaterials[2]->GetIndex();
    //
  case 2:
    if (recomp->mMaterials[1] && recomp->mMaterials[1]->GetID())
      passParams.TerrainSlot2 = recomp->mMaterials[1]->GetIndex();
    //
  case 1:
    if (recomp->mMaterials[0] && recomp->mMaterials[0]->GetID())
      passParams.TerrainSlot1 = recomp->mMaterials[0]->GetIndex();
    break;
  }



}

void RenderSystem::PushPassData()
{

    if (mActivePipeline)
        mActivePipeline->SetupPassTable(mPassTable);


    auto& groupBuf = RENDERMANAGER.GetGroupBuffer(mFrameCount);
    groupBuf->PassCustomTableInfo->PushGraphicsData(mPassTable.data(), sizeof(mPassTable));
}

void RenderSystem::PushInstanceData() {
  RENDERMANAGER.GetGroupBuffer(mFrameCount)
      ->InstanceInfo->PushGraphicsData(
          mInstanceVector.data(),
          static_cast<uint32>(mInstanceVector.size() * sizeof(RenderParams)));
}

void RenderSystem::PushLightData() {
  // light Component 추출
  const vector<Entity> &entities =
      mWorld->GetEntitiesWithComponent<LightComponent>();
  // ComponentPool<LightComponent>& lightComponents =
  // mWorld->GetComponentPool<LightComponent>();

  TransformComponent *transformComponent;
  LightComponent *lightComponent;
  RenderComponent *renderComponent;
  CameraComponent *cameraComponent;

  for (auto &entity : entities) {
    transformComponent = mWorld->GetComponent<TransformComponent>(entity);
    lightComponent = mWorld->GetComponent<LightComponent>(entity);
    renderComponent = mWorld->GetComponent<RenderComponent>(entity);
    cameraComponent = mWorld->GetComponent<CameraComponent>(entity);

    lightParams = {};

    lightParams.Color = lightComponent->mLightInfo.Color;
    lightParams.Position = lightComponent->mLightInfo.Position;
    lightParams.Direction = lightComponent->mLightInfo.Direction;
    lightParams.LightType = lightComponent->mLightInfo.LightType;
    lightParams.Range = lightComponent->mLightInfo.Range;
    lightParams.Angle = lightComponent->mLightInfo.Angle;

    lightParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
    lightParams.MatView = cameraComponent->mView.Transpose();
    lightParams.MatProjection = cameraComponent->mProjection.Transpose();
    lightParams.MatViewInv = cameraComponent->mView.Invert().Transpose();
    lightParams.MatProjectionInv = cameraComponent->mProjection.Invert().Transpose();

    mLightVector.push_back(lightParams);
  }
  passParams.LightsCount = static_cast<uint32>(mLightVector.size());
  RENDERMANAGER.GetGroupBuffer(mFrameCount)->LightInfo->
      PushGraphicsData(mLightVector.data(), static_cast<uint32>(sizeof(LightParams) * mLightVector.size()));



  auto lights =
      mWorld->GetEntitiesWithComponents<LightComponent, TransformComponent,
      CameraComponent, RenderComponent>();


  DrawBatch drawBatch{};
  for (auto& light : lights) {
      RenderComponent* lightComponent = mWorld->GetComponent<RenderComponent>(light);
      drawBatch.PSOShader = RESOURCEMANAGER.Get<Shader>(lightComponent->mMaterials[0]->GetShaderName());
      drawBatch.Mesh = lightComponent->mMesh;

      mLightDrawBatchs.push_back(drawBatch);
  }
}

void RenderSystem::PushObjectData() {
  const vector<EntityID> &gameObjects = mRenderComponentPool->GetEntities();
  auto View = mWorld->View<RenderComponent>();
  TransformComponent *transformComponent;
  AnimationComponent *animationComponent;
  RenderComponent *renderComponent;
  RenderParams renderParams{};

  uint32 index{};
  int32 index2{};

  for (auto gameObject : View) // 같은 머테리얼을 가진 것끼리 분류
  {
    // renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);

    /*if (mWorld->GetComponent<LightComponent>(gameObject) ||
    renderComponent->mIsNotObject) { continue;
    }*/

    //if (mWorld->GetComponent<TerrainComponent>(gameObject)) {
    //    continue;
    //  renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);
    //  transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
    //  TerrainComponent *terrainComponent = mWorld->GetComponent<TerrainComponent>(gameObject);

    //  objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
    //  mObjectVector.push_back(objectParams); // 트랜스폼 갱신

    //  renderComponent->mObjectIndex = index++; // objectParams의 index 지정
    //  index2 = -1;

    //  uint32 subMaterialIdx{};
    //  renderParams = {renderComponent->mObjectIndex,terrainComponent->mHeightmap->GetIndex(), index2, 0};
    //  mDeferredDrawItems.emplace_back(
    //      terrainComponent->mHeightmap->GetShader(), renderComponent->mMesh,
    //      terrainComponent->mHeightmap->GetShaderID(),
    //      renderComponent->mMesh->GetID(),
    //      terrainComponent->mHeightmap->GetID(), subMaterialIdx++,
    //      renderParams);

    //  continue;
    //}

    renderComponent = mRenderComponentPool->GetComponent(gameObject.GetID());
    transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
	animationComponent = mWorld->GetComponent<AnimationComponent>(gameObject);

    if (BulletComponent* bulletComponent = mWorld->GetComponent<BulletComponent>(gameObject)) {
        if (!bulletComponent->mIsActive)
            continue;
    }
    if (false == IsVisibleInFrustum(transformComponent, renderComponent)) {
      // 카메라 프러스텀 밖에서 라이트 프러스텀(구체) 테스트
      if (false == renderComponent->mVisibility) continue;

      const auto& obb = renderComponent->mWorldOBB; 
      const Vec3 objCenter(obb.Center.x, obb.Center.y, obb.Center.z);
      const float objRadius = sqrtf(obb.Extents.x * obb.Extents.x +
                                    obb.Extents.y * obb.Extents.y +
                                    obb.Extents.z * obb.Extents.z);


      const uint8 shadowCascadeMask =
          ComputeCascadeMask(objCenter, objRadius, transformComponent->mIsStatic);
      if (!shadowCascadeMask) continue;  // 어떤 cascade에도 속하지 않으면 생략

      // shadow-only 오브젝트: 트랜스폼 및 드로우 아이템 등록
      objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
      objectParams.Extra = Vec4(renderComponent->mOpacity, 0.f, 0.f, 0.f);
      mObjectVector.push_back(objectParams);
      renderComponent->mObjectIndex = index++;
      const int32 animIdx = animationComponent ? animationComponent->mAnimInstanceID : -1;

      uint32 shadowSubIdx{};
      const uint32 shadowSubMeshCount = renderComponent->mMesh ? renderComponent->mMesh->GetSubMeshCount() : 0;
      DrawItem shadowItem{};
      for (shared_ptr<Material>& material : renderComponent->mMaterials) {
        if (shadowSubIdx >= shadowSubMeshCount) break; // 서브메시 수 초과 머티리얼 스킵
        renderParams = {renderComponent->mObjectIndex, material->GetIndex(), animIdx, 0};
        shadowItem = {material->GetShader(), renderComponent->mMesh,
                      material->GetShaderID(), renderComponent->mMesh->GetID(),
                      material->GetID(), shadowSubIdx++, renderParams};
        // 교차하는 cascade에만 추가
        for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci) {
          if (shadowCascadeMask & static_cast<uint8>(1 << ci))
            mCascadeDrawItems[ci].push_back(shadowItem);
        }
      }
      continue;
    }
    if (false == renderComponent->mVisibility)
      continue;

    // Hit Flash: 피격 시 JHToon_PS에서 빨강으로 lerp되는 강도(0~1)
    // Extra.x = ObjectAlpha, Extra.y = HitFlashStrength, Extra.z = EmissiveGate
    float hitFlash = 0.f;
    if (auto* f = mWorld->GetComponent<HitFlashComponent>(gameObject))
      hitFlash = f->mCurrentStrength;

    // Emissive Gate
    float emissiveGate = 1.f;
    if (auto* re = mWorld->GetComponent<RhythmEmissiveComponent>(gameObject))
      emissiveGate = re->mCurrentGate;

    objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
    objectParams.Extra = Vec4(renderComponent->mOpacity, hitFlash, emissiveGate, renderComponent->mDissolve);
    mObjectVector.push_back(objectParams); // 트랜스폼 갱신

    renderComponent->mObjectIndex = index++; // objectParams의 index 지정

    animationComponent = mWorld->GetComponent<AnimationComponent>(gameObject);
    index2 = animationComponent ? animationComponent->mAnimInstanceID : -1;

    // 그림자 cascade 교차 마스크 — 프러스텀 내 오브젝트도 교차하는 cascade에만 그림
    uint8 visCascadeMask = 0;
    if (renderComponent->mCheckFrustum) {
      // IsVisibleInFrustum에서 mWorldOBB가 이미 갱신된 상태
      const auto& vObb = renderComponent->mWorldOBB;
      const Vec3 vCenter(vObb.Center.x, vObb.Center.y, vObb.Center.z);
      const float vRadius = sqrtf(vObb.Extents.x * vObb.Extents.x +
                                  vObb.Extents.y * vObb.Extents.y +
                                  vObb.Extents.z * vObb.Extents.z);
      visCascadeMask = ComputeCascadeMask(vCenter, vRadius, transformComponent->mIsStatic);
    } else {
      // OBB 미갱신(mCheckFrustum=false) 오브젝트는 보수적으로 이동 cascade 전부 (기존 동작 유지)
      for (uint32 ci = 0; ci < SHADOW_MAP_CASCADE_INDEX; ++ci)
        if (mCascadeActive[ci]) visCascadeMask |= static_cast<uint8>(1 << ci);
      if (mMapCascadeDirty && transformComponent->mIsStatic && mCascadeActive[SHADOW_MAP_CASCADE_INDEX])
        visCascadeMask |= static_cast<uint8>(1 << SHADOW_MAP_CASCADE_INDEX);
    }

    uint32 subMaterialIdx{};

    const uint32 subMeshCount = renderComponent->mMesh ? renderComponent->mMesh->GetSubMeshCount() : 0;
    DrawItem drawItem{};
    for (shared_ptr<Material> &material : renderComponent->mMaterials) {
      if (subMaterialIdx >= subMeshCount) break;        // 메시 서브메시(인덱스버퍼) 수보다 머티리얼이 많으면

      // EffectFlagComponent가 있으면 object3에 플래그 주입 (PSO 변경 없이 쉐이더 분기)
      uint32 effectFlag = 0;
      if (auto* efx = mWorld->GetComponent<EffectFlagComponent>(gameObject))
          effectFlag = static_cast<uint32>(efx->mFlag);

      renderParams = {renderComponent->mObjectIndex, material->GetIndex(),
                      index2, effectFlag};

      drawItem = { material->GetShader(),
                  renderComponent->mMesh,
                   material->GetShaderID(),
                  renderComponent->mMesh->GetID(),
                  material->GetID(),
                  subMaterialIdx++,
                  renderParams};
      mDeferredDrawItems.push_back(drawItem);


      for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci) {
        if (visCascadeMask & static_cast<uint8>(1 << ci))
          mCascadeDrawItems[ci].push_back(drawItem);
      }
      /*mDeferredDrawItems.emplace_back(
              material->GetShader(),
              renderComponent->mMesh,
              material->GetShaderID(),
              renderComponent->mMesh->GetID(),
              material->GetID(),
              subMaterialIdx++,
              renderParams
      );*/
    }
  }

  // collison box
  if (sDrawColliders && mWireCube) {
    auto colliderEntities =
        mWorld->GetEntitiesWithComponents<TransformComponent,
                                          BoxColliderComponent>();
    for (auto e : colliderEntities) {
      auto *tr = mWorld->GetComponent<TransformComponent>(e);
      auto *col = mWorld->GetComponent<BoxColliderComponent>(e);
      auto *render = mWorld->GetComponent<RenderComponent>(e);
      if (!col || !col->bDebugDraw)
        continue;
      if (render && !render->mVisibility)
        continue;

      
      Matrix colliderLocal = Matrix::CreateScale(col->mHalfExtents * 2.0f) *
                             Matrix::CreateTranslation(col->mCenter);

      // [수정] 스케일 없는 월드에 붙인다 -> 엔티티 Scale에 영향 받지 않음
      Matrix boxWorld = colliderLocal * tr->mWorldMatrix;

      objectParams.MatWorld = boxWorld.Transpose();
      objectParams.Extra = Vec4(1.f, 0.f, 0.f, 0.f);
      mObjectVector.push_back(objectParams);

      const uint32 objIndex = index++;
      const int32 animId = -1;

      shared_ptr<Material> mat = nullptr;
      if (col->bIsColliding)
        mat = mDebugLineRedMat;
      else
        mat = mDebugLineGreenMat;

      if (!mat)
        continue;

      mDeferredDrawItems.emplace_back(
          mat->GetShader(), mWireCube, mat->GetShaderID(), mWireCube->GetID(),
          mat->GetID(), 0, RenderParams{objIndex, mat->GetIndex(), animId, 0});
    }
  }

  // 프러스텀 컬링 OBB 시각화
  if (sDrawCullingOBB && mWireCube) {
    auto obbEntities =
        mWorld->GetEntitiesWithComponents<TransformComponent, RenderComponent>();
    for (auto e : obbEntities) {
      auto *render = mWorld->GetComponent<RenderComponent>(e);
      if (!render || !render->mCheckFrustum)
        continue;  // 컬링 대상 엔티티만
      if (!render->mVisibility)
        continue;

      auto *tr = mWorld->GetComponent<TransformComponent>(e);
      if (!tr)
        continue;

      // 메인 루프 호출 순서에 의존하지 않도록 직접 갱신하게 했음
      render->UpdateWorldOBB(tr);

      const BoundingOrientedBox &obb = render->mWorldOBB;
      Matrix boxWorld =
          Matrix::CreateScale(Vec3(obb.Extents.x, obb.Extents.y, obb.Extents.z) * 2.0f) *
          Matrix::CreateFromQuaternion(Quaternion(obb.Orientation.x, obb.Orientation.y,
                                                  obb.Orientation.z, obb.Orientation.w)) *
          Matrix::CreateTranslation(Vec3(obb.Center.x, obb.Center.y, obb.Center.z));

      objectParams.MatWorld = boxWorld.Transpose();
      objectParams.Extra = Vec4(1.f, 0.f, 0.f, 0.f);
      mObjectVector.push_back(objectParams);
      const uint32 objIndex = index++;

      shared_ptr<Material> mat = mDebugLineGreenMat;
      if (!mat)
        continue;

      mDeferredDrawItems.emplace_back(
          mat->GetShader(), mWireCube, mat->GetShaderID(), mWireCube->GetID(),
          mat->GetID(), 0, RenderParams{objIndex, mat->GetIndex(), -1, 0});
    }
  }

  // ─────────────────────────────────────────────────────
  // 디버그 라인 렌더링 (NavMesh 경로, 기타 SubmitDebugLine 호출)
  // 단위 선분 메쉬 [(0,0,0)->(1,0,0)]에 월드 행렬로 임의의 선분을 표현
  // ─────────────────────────────────────────────────────
  if (mLineMesh && !sDebugLineQueue.empty()) {
    for (const auto& req : sDebugLineQueue) {
      // (0,0,0) : start, (1,0,0) : end 가 되도록 월드 행렬 구성
      // row 0 = 방향 벡터(B-A), row 3 = 시작점(A)
      Vec3 dir = req.end - req.start;
      Matrix world(
        dir.x,       dir.y,       dir.z,       0.f,
        0.f,         1.f,         0.f,         0.f,
        0.f,         0.f,         1.f,         0.f,
        req.start.x, req.start.y, req.start.z, 1.f
      );

      objectParams.MatWorld = world.Transpose();
      objectParams.Extra = Vec4(1.f, 0.f, 0.f, 0.f);
      mObjectVector.push_back(objectParams);
      const uint32 objIdx = index++;

      // 색상에 따라 머티리얼 선택
      shared_ptr<Material> mat;
      if (req.color.x > 0.5f && req.color.y > 0.5f && req.color.z < 0.5f)
        mat = mDebugLineYellowMat;
      else if (req.color.x > 0.5f && req.color.y < 0.5f)
        mat = mDebugLineRedMat;
      else if (req.color.y > 0.5f && req.color.x < 0.5f)
        mat = mDebugLineGreenMat;
      else
        mat = mDebugLineNoDepthMat;

      if (!mat) continue;

      mDeferredDrawItems.emplace_back(
        mat->GetShader(), mLineMesh,
        mat->GetShaderID(), mLineMesh->GetID(),
        mat->GetID(), 0,
        RenderParams{ objIdx, mat->GetIndex(), -1, 0 });
    }
    sDebugLineQueue.clear();
  }

  // std::sort(mDeferredDrawItems.begin(), mDeferredDrawItems.end(), [](auto& a,
  // auto& b) { 	if (a.PSOID != b.PSOID) return a.PSOID < b.PSOID;   //
  // PSO 우선
  //	if (a.MeshID != b.MeshID) return a.MeshID < b.MeshID;  // Mesh
  //	return a.SubMesh < b.SubMesh;                          // Submesh
  //	});

  // [수정] 정렬 키를 실제 드로우에 사용되는 SubMeshIndex로 맞춘다.
  std::sort(mDeferredDrawItems.begin(), mDeferredDrawItems.end(),
            [](const DrawItem &a, const DrawItem &b) {
              if (a.PSOID != b.PSOID)
                return a.PSOID < b.PSOID;
              if (a.MeshID != b.MeshID)
                return a.MeshID < b.MeshID;

              // [수정] a.SubMeshIndex 기준으로 묶어야 InstancingRender의
              // Render(submesh)와 일치한다.
              if (a.SubMeshIndex != b.SubMeshIndex)
                return a.SubMeshIndex < b.SubMeshIndex;

              // (선택) material id까지 묶고 싶으면 여기서 비교
              // return a.MaterialID < b.MaterialID;

              return a.SubMesh < b.SubMesh;
            });

  uint32 psoId{};
  uint32 meshId{};
  uint32 smIdx{};
  uint32 base{};

  for (uint32 i = 0; i < mDeferredDrawItems.size();) {
    psoId = mDeferredDrawItems[i].PSOID;
    meshId = mDeferredDrawItems[i].MeshID;
    smIdx = mDeferredDrawItems[i].SubMesh;

    // 이 배치의 인스턴스들을 전역 InstanceGPU[]에 연속으로 복사
    base =
        (uint32)mInstanceVector.size(); //-i;	// 수식이 이게 맞나? 확인 필요
    uint32 j = i;

    while (j < mDeferredDrawItems.size() &&
           mDeferredDrawItems[j].PSOID == psoId &&
           mDeferredDrawItems[j].MeshID == meshId &&
           mDeferredDrawItems[j].SubMesh == smIdx) {
      mInstanceVector.push_back(mDeferredDrawItems[j].InstanceGPU);
      ++j;
    }

    mBatch.PSOShader = mDeferredDrawItems[i].PSOShader;
    mBatch.Mesh = mDeferredDrawItems[i].PMesh;
    mBatch.PSOID = psoId;
    // mBatch.MeshID = meshId;
    // mBatch.SubMesh = smIdx;
    mBatch.SubMeshIndex = mDeferredDrawItems[i].SubMeshIndex;
    mBatch.BaseInstance = base;
    mBatch.InstanceCount = (j - i); // 이 run의 인스턴스 수
    // mBatch.InstanceGPU = mDeferredDrawItems[i].InstanceGPU;
    // mBatch.ParamsINX = i;
    mDeferredDrawBatchs.push_back(mBatch);

    i = j; // 다음 run으로
  }

  // cascade별 배치 생성 (mCascadeDrawItems[ci] → mCascadeDrawBatchs[ci])
  for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci) {
    if (!mCascadeActive[ci] || mCascadeDrawItems[ci].empty()) continue;

    std::sort(mCascadeDrawItems[ci].begin(), mCascadeDrawItems[ci].end(),
              [](const DrawItem& a, const DrawItem& b) {
                if (a.PSOID != b.PSOID) return a.PSOID < b.PSOID;
                if (a.MeshID != b.MeshID) return a.MeshID < b.MeshID;
                if (a.SubMeshIndex != b.SubMeshIndex) return a.SubMeshIndex < b.SubMeshIndex;
                return a.SubMesh < b.SubMesh;
              });

    for (uint32 i = 0; i < mCascadeDrawItems[ci].size();) {
      psoId  = mCascadeDrawItems[ci][i].PSOID;
      meshId = mCascadeDrawItems[ci][i].MeshID;
      smIdx  = mCascadeDrawItems[ci][i].SubMesh;
      base   = (uint32)mInstanceVector.size();
      uint32 j = i;

      while (j < mCascadeDrawItems[ci].size() &&
             mCascadeDrawItems[ci][j].PSOID  == psoId &&
             mCascadeDrawItems[ci][j].MeshID == meshId &&
             mCascadeDrawItems[ci][j].SubMesh == smIdx) {
        mInstanceVector.push_back(mCascadeDrawItems[ci][j].InstanceGPU);
        ++j;
      }

      mBatch.PSOShader    = mCascadeDrawItems[ci][i].PSOShader;
      mBatch.Mesh         = mCascadeDrawItems[ci][i].PMesh;
      mBatch.PSOID        = psoId;
      mBatch.SubMeshIndex = mCascadeDrawItems[ci][i].SubMeshIndex;
      mBatch.BaseInstance = base;
      mBatch.InstanceCount = (j - i);
      mCascadeDrawBatchs[ci].push_back(mBatch);

      i = j;
    }
  }

  RENDERMANAGER.GetGroupBuffer(mFrameCount)
      ->ObjectInfo->PushGraphicsData(
          mObjectVector.data(),
          static_cast<uint32>(sizeof(objectParams) * mObjectVector.size()));
}


void RenderSystem::PushShadowCascades() {
    mHasDirectionalShadow = false;

    for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci) {
        mCascadeActive[ci] = false;
        CascadeSplit[ci] = 0.f;
        passParams.CascadeShadowVP[ci] = Matrix::Identity.Transpose();
    }


    for (auto& light : mWorld->GetEntitiesWithComponent<LightComponent>()) {
        LightComponent* lightComponent = mWorld->GetComponent<LightComponent>(light);
        if (lightComponent->mLightInfo.LightType !=
            static_cast<int32>(LIGHT_TYPE::DIRECTIONAL_LIGHT))
            continue;
        UpdateCascadeShadowMatrices(lightComponent);
        mHasDirectionalShadow = true;
        break;
    }
    if (!mHasDirectionalShadow)
        passParams.CascadeSplitDistances = Vec4(0.f, 0.f, 0.f, 0.f);
}

void RenderSystem::UpdateCascadeShadowMatrices(LightComponent *lightComponent) {
    const float cameraNear = max(mCamera->mShadowNear, mCamera->mNear);
    const float cameraFar = min(max(mCamera->mShadowFar, cameraNear + 0.001f), mCamera->mFar);

  const float nearForSplit = max(cameraNear, 0.001f);
  const float farForSplit = max(cameraFar, nearForSplit + 0.001f);
  const float ratio = farForSplit / nearForSplit;
  const float lambda = min(max(mCascadeSplitLambda, 0.0f), 1.0f);
  // 카메라 추종(이동) cascade는 마지막(맵 고정) 슬라이스를 제외한 개수 — 0..mShadowFar를 분할
  constexpr float movingCascadeCount = static_cast<float>(SHADOW_MAP_CASCADE_INDEX);

  for (uint32 i = 0; i < SHADOW_MAP_CASCADE_INDEX; ++i) {
      const float p = static_cast<float>(i + 1) / movingCascadeCount;
      const float logSplit = nearForSplit * powf(ratio, p);
      const float uniformSplit = nearForSplit + (farForSplit - nearForSplit) * p;
      CascadeSplit[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
  }
  // 맵 고정 cascade는 카메라 far까지 커버 — 셰이더 SelectCascadeIndex가 splits.z 기준으로 선택
  CascadeSplit[SHADOW_MAP_CASCADE_INDEX] = mCamera->mFar;


  const Matrix invProj = mCamera->mProjection.Invert();
  const Matrix invView = mCamera->mView.Invert();
  // frustumNearView/FarView는 invProj(실제 투영행렬) 기반으로 mNear~mFar 범위를 커버한다.
  // nearT/farT의 분모도 동일하게 실제 near/far 범위를 사용해야
  // lerp 결과 z가 splitNear/splitFar와 일치한다.
  const float cameraRange = max(mCamera->mFar - mCamera->mNear, 0.001f);
  const array<Vec3, 4> ndcNearCorners = {
      Vec3(-1.f, 1.f, 0.f), Vec3(1.f, 1.f, 0.f), Vec3(1.f, -1.f, 0.f),Vec3(-1.f, -1.f, 0.f)};
  const array<Vec3, 4> ndcFarCorners = {
      Vec3(-1.f, 1.f, 1.f), Vec3(1.f, 1.f, 1.f), Vec3(1.f, -1.f, 1.f),Vec3(-1.f, -1.f, 1.f)};

  array<Vec3, 4> frustumNearView{};
  array<Vec3, 4> frustumFarView{};
  for (uint32 i = 0; i < 4; ++i) {
    frustumNearView[i] = Vec3::Transform(ndcNearCorners[i], invProj);
    frustumFarView[i] = Vec3::Transform(ndcFarCorners[i], invProj);
  }

  Vec3 lightDir = Vec3(lightComponent->mLightInfo.Direction);


  if (lightDir.LengthSquared() < 1e-4f)
    lightDir = Vec3(0.f, -1.f, 0.f);
  lightDir.Normalize();




  for (uint32 cascadeIndex = 0; cascadeIndex < SHADOW_MAP_CASCADE_INDEX; ++cascadeIndex) {

    const float splitNear =
        (cascadeIndex == 0) ? cameraNear
        : min(CascadeSplit[cascadeIndex - 1], cameraFar);
    const float splitFar = min(CascadeSplit[cascadeIndex], cameraFar);
    if (splitFar <= splitNear) {
        mCascadeActive[cascadeIndex] = false;
      passParams.CascadeShadowVP[cascadeIndex] = Matrix::Identity.Transpose();
      continue;
    }
    mCascadeActive[cascadeIndex] = true;
    const float nearT = min(max((splitNear - mCamera->mNear) / cameraRange, 0.0f), 1.0f);
    const float farT = min(max((splitFar - mCamera->mNear) / cameraRange, 0.0f), 1.0f);

    array<Vec3, 8> worldCorners{};
    Vec3 frustumCenter = Vec3::Zero;
    for (uint32 i = 0; i < 4; ++i) {
      const Vec3 nearCornerView = Vec3::Lerp(frustumNearView[i], frustumFarView[i], nearT);
      const Vec3 farCornerView = Vec3::Lerp(frustumNearView[i], frustumFarView[i], farT);

      worldCorners[i] = Vec3::Transform(nearCornerView, invView);
      worldCorners[i + 4] = Vec3::Transform(farCornerView, invView);

      frustumCenter += worldCorners[i] + worldCorners[i + 4];
    }
    frustumCenter /= 8.f;


    Vec3 centerView = Vec3::Zero;
    for (uint32 i = 0; i < 4; ++i) {
      centerView += Vec3::Lerp(frustumNearView[i], frustumFarView[i], nearT);
      centerView += Vec3::Lerp(frustumNearView[i], frustumFarView[i], farT);
    }
    centerView /= 8.f;

    float radius = 0.f;
    for (uint32 i = 0; i < 4; ++i) {
      radius = max(radius, (Vec3::Lerp(frustumNearView[i], frustumFarView[i], nearT) - centerView).Length());
      radius = max(radius, (Vec3::Lerp(frustumNearView[i], frustumFarView[i], farT) - centerView).Length());
    }
    radius = max(ceil(radius * 16.f) / 16.f, 1.f);
    radius *= 1.05f;

    // 라이트 프러스텀 구체 저장 (shadow-only 컬링용)
    mCascadeFrustumCenter[cascadeIndex] = frustumCenter;
    mCascadeFrustumRadius[cascadeIndex] = radius;

    const Vec3 up = abs(lightDir.Dot(Vec3::Up)) > 0.99f ? Vec3::Right : Vec3::Up;
    const Vec3 eye = frustumCenter - lightDir * (radius * 2.f);
    Matrix lightView = Matrix::CreateLookAt(eye, frustumCenter, up);

    // Light 공간 XY 축 저장 — AABB 컬링에 사용 (행렬 Row 0/1 = Right/Up)
    mCascadeLightRight[cascadeIndex] = Vec3(lightView._11, lightView._12, lightView._13);
    mCascadeLightUp[cascadeIndex]    = Vec3(lightView._21, lightView._22, lightView._23);


    Vec3 centerLS = Vec3::Transform(frustumCenter, lightView);
    const float texelWorldSize = max((radius * 2.f) / shadowMapSize, 1e-5f);
    centerLS.x = floor(centerLS.x / texelWorldSize + 0.5f) * texelWorldSize;
    centerLS.y = floor(centerLS.y / texelWorldSize + 0.5f) * texelWorldSize;

    const Matrix invLightView = lightView.Invert();
    const Vec3 snappedCenterWorld = Vec3::Transform(centerLS, invLightView);

    // [수정] 스냅된 월드 중심으로 lightView 재생성
    const Vec3 snappedEye = snappedCenterWorld - lightDir * (radius * 2.f);
    lightView = Matrix::CreateLookAt(snappedEye, snappedCenterWorld, up);

    Vec3 minExtents(centerLS.x - radius, centerLS.y - radius, FLT_MAX);
    Vec3 maxExtents(centerLS.x + radius, centerLS.y + radius, -FLT_MAX);

    float minZ = FLT_MAX;
    float maxZ = -FLT_MAX;
    for (Vec3 &corner : worldCorners) {
        const Vec3 cornerLS = Vec3::Transform(corner, lightView);
        minZ = min(minZ, cornerLS.z);
        maxZ = max(maxZ, cornerLS.z);
    }

    minExtents.z = minZ;
    maxExtents.z = maxZ;


    const float terrainHeightRange = 0.0f;
    const float zPadding = max(max(radius * 4.f, 50.f), terrainHeightRange);
    const float nearPlane = minExtents.z - zPadding;
    const float farPlane = maxExtents.z + zPadding;
    Matrix lightProj = Matrix::CreateOrthographicOffCenter(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, nearPlane, farPlane);
    passParams.CascadeShadowVP[cascadeIndex] = (lightView * lightProj).Transpose();
  }

  // 마지막 슬라이스— dirty일 때만 VP 재계산
  UpdateMapCascade(lightDir);


  passParams.CascadeSplitDistances =
      Vec4(CascadeSplit[0], CascadeSplit[1], CascadeSplit[2], mCamera->mFar);

}

void RenderSystem::UpdateMapCascade(const Vec3& lightDir)
{
  constexpr uint32 mapIdx = SHADOW_MAP_CASCADE_INDEX;

  // 정적 캐스터 수 집계 — dirty 검출용 (AABB 계산은 dirty 프레임에만)
  uint32 staticCount = 0;
  auto entities = mWorld->GetEntitiesWithComponents<TransformComponent, RenderComponent>();

  for (Entity e : entities) {
    TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);
    RenderComponent* rc = mWorld->GetComponent<RenderComponent>(e);

    if (!tr->mIsStatic || !rc->mVisibility || !rc->mMesh) continue;
    ++staticCount;
  }

  // dirty 트리거: 라이트 방향 변화 / 정적 캐스터 수 변화 / 첫 프레임(초기값 true)
  if ((lightDir - mPrevLightDir).LengthSquared() > 1e-6f || staticCount != mPrevStaticCasterCount)
    mMapCascadeDirty = true;

  mPrevLightDir = lightDir;
  mPrevStaticCasterCount = staticCount;

  if (staticCount == 0) {
    mCascadeActive[mapIdx] = false;
    passParams.CascadeShadowVP[mapIdx] = Matrix::Identity.Transpose();
    return;
  }

  mCascadeActive[mapIdx] = true;

  if (mMapCascadeDirty) {
    // 정적 캐스터 전체의 월드 AABB
    Vec3 aabbMin(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 aabbMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);


    for (Entity e : entities) {
      TransformComponent* tr = mWorld->GetComponent<TransformComponent>(e);
      RenderComponent* rc = mWorld->GetComponent<RenderComponent>(e);
      if (!tr->mIsStatic || !rc->mVisibility || !rc->mMesh) continue;

      rc->UpdateWorldOBB(tr);  // 첫 프레임 등 OBB 미갱신 상태 대비
      XMFLOAT3 corners[BoundingOrientedBox::CORNER_COUNT];
      rc->mWorldOBB.GetCorners(corners);

      for (const XMFLOAT3& c : corners) {
        aabbMin = Vec3::Min(aabbMin, Vec3(c.x, c.y, c.z));
        aabbMax = Vec3::Max(aabbMax, Vec3(c.x, c.y, c.z));
      }
    }

    const Vec3 center = (aabbMin + aabbMax) * 0.5f;
    const float radius = ((aabbMax - aabbMin) * 0.5f).Length();

    const Vec3 up = abs(lightDir.Dot(Vec3::Up)) > 0.99f ? Vec3::Right : Vec3::Up;
    const Vec3 eye = center - lightDir * (radius * 2.f);
    const Matrix lightView = Matrix::CreateLookAt(eye, center, up);

    // AABB 8코너를 light space로 변환해 정확한 ortho 범위 산출
    Vec3 minLS(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxLS(-FLT_MAX, -FLT_MAX, -FLT_MAX);


    const Vec3 cornersW[8] = {
        {aabbMin.x, aabbMin.y, aabbMin.z}, {aabbMax.x, aabbMin.y, aabbMin.z},
        {aabbMin.x, aabbMax.y, aabbMin.z}, {aabbMax.x, aabbMax.y, aabbMin.z},
        {aabbMin.x, aabbMin.y, aabbMax.z}, {aabbMax.x, aabbMin.y, aabbMax.z},
        {aabbMin.x, aabbMax.y, aabbMax.z}, {aabbMax.x, aabbMax.y, aabbMax.z},
    };


    for (const Vec3& cw : cornersW) {
      const Vec3 ls = Vec3::Transform(cw, lightView);
      minLS = Vec3::Min(minLS, ls);
      maxLS = Vec3::Max(maxLS, ls);
    }

    // 고정 VP라 texel snapping 불필요. 경계 여유만 소량 부여
    const Vec3 margin = (maxLS - minLS) * 0.05f;
    const Matrix lightProj = Matrix::CreateOrthographicOffCenter(
        minLS.x - margin.x, maxLS.x + margin.x,
        minLS.y - margin.y, maxLS.y + margin.y,
        minLS.z - margin.z, maxLS.z + margin.z);

    mMapCascadeVP = lightView * lightProj;

    // 캐스터 컬링 데이터 (ComputeCascadeMask 공용)
    mCascadeFrustumCenter[mapIdx] = center;
    mCascadeFrustumRadius[mapIdx] = radius * 1.05f;
    mCascadeLightRight[mapIdx] = Vec3(lightView._11, lightView._12, lightView._13);
    mCascadeLightUp[mapIdx]    = Vec3(lightView._21, lightView._22, lightView._23);
  }

  // VP는 고정값이므로 dirty 여부와 무관하게 매 프레임 동일 값 업로드
  passParams.CascadeShadowVP[mapIdx] = mMapCascadeVP.Transpose();
}



uint8 RenderSystem::ComputeCascadeMask(const Vec3& objCenter, float objRadius, bool isStatic) const
{
  uint8 mask = 0;
  for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci) {
    if (!mCascadeActive[ci]) continue;  
    // 캐시
    if (ci == SHADOW_MAP_CASCADE_INDEX && (!isStatic || !mMapCascadeDirty)) continue;

    const Vec3 toObj      = objCenter - mCascadeFrustumCenter[ci];
    const float projX     = fabsf(toObj.Dot(mCascadeLightRight[ci]));
    const float projY     = fabsf(toObj.Dot(mCascadeLightUp[ci]));
    const float halfExtent = mCascadeFrustumRadius[ci] + objRadius;

    if (projX < halfExtent && projY < halfExtent)
      mask |= static_cast<uint8>(1 << ci);
  }
  return mask;
}






bool RenderSystem::IsVisibleInFrustum(TransformComponent *trans, RenderComponent *renderComponent)
{
  if (renderComponent->mCheckFrustum && mCamera) {
    // if (trans->mIsDirty)
    renderComponent->UpdateWorldOBB(trans);

    if (!mCamera->IntersectsOBB(renderComponent->mWorldOBB)) {
      return false;
    }
  }
  return true;
}


void RenderSystem::InstancingRender(DrawBatch &drawBatch) {

  drawBatch.Mesh->Render(drawBatch.InstanceCount, drawBatch.SubMeshIndex, 
      0, 0 /*drawBatch.SubMeshIndex+ drawBatch.ParamsINX*/);
}
