#include "pch.h"

#include "RenderSystem.h"
#include "AnimationComponent.h"
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

#include "ShadowPass.h"
#include "GBufferPass.h"
#include "LightsPass.h"
#include "ForwardPass.h"

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
  mDebugLineMat = RESOURCEMANAGER.Get<Material>(L"DebugLine");
  mDebugLineNoDepthMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_NoDepth");

  mDebugLineGreenMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_Green");
  mDebugLineRedMat = RESOURCEMANAGER.Get<Material>(L"DebugLine_Red");

  mDeferredDrawItems.reserve(1000);
  mDeferredDrawBatchs.reserve(1000);
  mInstanceVector.reserve(1000);

  mShadowPass = make_shared<ShadowPass>();
  mGBufferPass = make_shared<GBufferPass>();
  mLightPass = make_shared<LightsPass>();
  
  mForwardPass = make_shared<ForwardPass>();
}

void RenderSystem::Update() {
  if (false == mWorld->HasComponentPool<MainCameraComponent>())
    return;
  /*std::vector<Entity> camera{
  mWorld->GetEntitiesWithComponent<MainCameraComponent>()[0]}; mCamera =
  mWorld->GetComponent<CameraComponent>(camera[0]);*/
  auto cameraView = mWorld->View<MainCameraComponent>();
  auto cameraIt = cameraView.begin();
  if (cameraIt == cameraView.end()) {
    return;
  }
  mCamera = mWorld->GetComponent<CameraComponent>(*cameraIt);

  ClearRTV();

  PushData();

  RenderShadow();
  RenderDeferred();
  RenderForward();
  RenderPost();
  // m_postStack -> update();
}

void RenderSystem::PushData() {
  RENDERMANAGER.SetGraphicsTable();
  PushLandData();
  PushCubeData();

  PushObjectData();
  PushLightData();
  PushPassData();
  PushInstanceData();
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
  }else {
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).ClearRenderTargetView(backIndex);
  }

  // Shadow Group 초기화
  //RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SHADOW)).ClearRenderTargetView();

  // Deferred Group 초기화
  RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::G_BUFFER)).ClearRenderTargetView();

  // Lighting Group 초기화
  RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::LIGHTING)).ClearRenderTargetView();

  mFrameCount = RENDERMANAGER.GetFrameResourceIndex();

  ClearBuffer();
  // 추후 lightvector관련들 clear도 모두 확인할껏.
}

void RenderSystem::ClearBuffer() {
  mCurrPSOID = 0;

  mDeferredDrawItems.clear();
  mDeferredDrawBatchs.clear();
  mLightDrawBatchs.clear();
  mInstanceVector.clear();
  // passParams = {};

  mLightVector.clear();
  mObjectVector.clear();
  mDummyVector.clear();
}

void RenderSystem::PushPassData() {

  passParams.MatView = mCamera->mView.Transpose();
  passParams.MatProjection = mCamera->mProjection.Transpose();
  passParams.MatViewInv = mCamera->mView.Invert().Transpose();
  passParams.MatProjectionInv = mCamera->mProjection.Invert().Transpose();
  passParams.ScreenSize = {
      static_cast<float>(RENDERMANAGER.GetWindow().Width),
      static_cast<float>(RENDERMANAGER.GetWindow().Height)};
  passParams.CascadeSplitDistances =
      Vec4(CascadeSplit[0], CascadeSplit[1], CascadeSplit[2], CascadeSplit[3]);

  for (auto& light : mWorld->GetEntitiesWithComponent<LightComponent>()) {
      LightComponent* lightComponent = mWorld->GetComponent<LightComponent>(light);
      if (lightComponent->mLightInfo.LightType !=
          static_cast<int32>(LIGHT_TYPE::DIRECTIONAL_LIGHT))
          continue;

      UpdateCascadeShadowMatrices(lightComponent);
  }


  shared_ptr<GroupBuffer> groupBuffer =RENDERMANAGER.GetGroupBuffer(mFrameCount);

  groupBuffer->PassInfo->PushData(&passParams, sizeof(PassParams));
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
  passParams.SkyBoxIndex = 0;
  shared_ptr<Material> skyboxMaterial =
      RESOURCEMANAGER.Get<Material>(L"Skybox");
  if (skyboxMaterial) {
    shared_ptr<Texture> skyboxTexture =
        skyboxMaterial->GetTexture(DIFFUSEMAP0INDEX);
    if (skyboxTexture && skyboxTexture->IsCubeMap())
      passParams.SkyBoxIndex = skyboxTexture->GetCubeMapIndex();
  }
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
  case 5:
    if (recomp->mMaterials[4] && recomp->mMaterials[4]->GetID())
      passParams.TerrainSlot5 = recomp->mMaterials[4]->GetIndex();
  case 4:
    if (recomp->mMaterials[3] && recomp->mMaterials[3]->GetID())
      passParams.TerrainSlot4 = recomp->mMaterials[3]->GetIndex();
  case 3:
    if (recomp->mMaterials[2] && recomp->mMaterials[2]->GetID())
      passParams.TerrainSlot3 = recomp->mMaterials[2]->GetIndex();
  case 2:
    if (recomp->mMaterials[1] && recomp->mMaterials[1]->GetID())
      passParams.TerrainSlot2 = recomp->mMaterials[1]->GetIndex();
  case 1:
    if (recomp->mMaterials[0] && recomp->mMaterials[0]->GetID())
      passParams.TerrainSlot1 = recomp->mMaterials[0]->GetIndex();
    break;
  }



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
  // push 시작 (vector에 값 밀어 넣기)
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

    if (mWorld->GetComponent<TerrainComponent>(gameObject)) {

      renderComponent = mWorld->GetComponent<RenderComponent>(gameObject);
      transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);
      TerrainComponent *terrainComponent = mWorld->GetComponent<TerrainComponent>(gameObject);

      objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
      mObjectVector.push_back(objectParams); // 트랜스폼 갱신

      renderComponent->mObjectIndex = index++; // objectParams의 index 지정
      index2 = -1;

      uint32 subMaterialIdx{};
      renderParams = {renderComponent->mObjectIndex,terrainComponent->mHeightmap->GetIndex(), index2, 0};
      mDeferredDrawItems.emplace_back(
          terrainComponent->mHeightmap->GetShader(), renderComponent->mMesh,
          terrainComponent->mHeightmap->GetShaderID(),
          renderComponent->mMesh->GetID(),
          terrainComponent->mHeightmap->GetID(), subMaterialIdx++,
          renderParams);

      continue;
    }

    renderComponent = mRenderComponentPool->GetComponent(gameObject.GetID());
    transformComponent = mWorld->GetComponent<TransformComponent>(gameObject);

    if (false == IsFrustumCulled(transformComponent, renderComponent))
      continue;
    if (false == renderComponent->mVisibility)
      continue;

    objectParams.MatWorld = transformComponent->mWorldMatrix.Transpose();
    mObjectVector.push_back(objectParams); // 트랜스폼 갱신

    renderComponent->mObjectIndex = index++; // objectParams의 index 지정

    animationComponent = mWorld->GetComponent<AnimationComponent>(gameObject);
    index2 = animationComponent ? animationComponent->mAnimInstanceID : -1;

    uint32 subMaterialIdx{};
    DrawItem drawItem{};
    for (shared_ptr<Material> &material : renderComponent->mMaterials) {

      renderParams = {renderComponent->mObjectIndex, material->GetIndex(),
                      index2, 0};

      drawItem = {material->GetShader(),
                  renderComponent->mMesh,
                  material->GetShaderID(),
                  renderComponent->mMesh->GetID(),
                  material->GetID(),
                  subMaterialIdx++,
                  renderParams};
      mDeferredDrawItems.push_back(drawItem);
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
  if (mWireCube) {
    auto colliderEntities =
        mWorld->GetEntitiesWithComponents<TransformComponent,
                                          BoxColliderComponent>();
    for (auto e : colliderEntities) {
      auto *tr = mWorld->GetComponent<TransformComponent>(e);
      auto *col = mWorld->GetComponent<BoxColliderComponent>(e);
      if (!col || !col->bDebugDraw)
        continue;

      
      Matrix colliderLocal = Matrix::CreateScale(col->mHalfExtents * 2.0f) *
                             Matrix::CreateTranslation(col->mCenter);

      // [수정] 스케일 없는 월드에 붙인다 -> 엔티티 Scale에 영향 받지 않음
      Matrix boxWorld = colliderLocal * tr->mWorldMatrix;

      objectParams.MatWorld = boxWorld.Transpose();
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

  RENDERMANAGER.GetGroupBuffer(mFrameCount)
      ->ObjectInfo->PushGraphicsData(
          mObjectVector.data(),
          static_cast<uint32>(sizeof(objectParams) * mObjectVector.size()));
}


void RenderSystem::UpdateCascadeShadowMatrices(LightComponent *lightComponent) {
  const float cameraNear = mCamera->mNear;
  const float cameraFar = mCamera->mFar;
  const Matrix invProj = mCamera->mProjection.Invert();
  const Matrix invView = mCamera->mView.Invert();
  const float cameraRange = max(cameraFar - cameraNear, 0.001f);
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


  constexpr float shadowMapSize = 4096.f;


  for (uint32 cascadeIndex = 0;
       cascadeIndex < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++cascadeIndex) {
     
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
    const float nearT = (splitNear - cameraNear) / cameraRange;
    const float farT = (splitFar - cameraNear) / cameraRange;

    array<Vec3, 8> worldCorners{};
    Vec3 frustumCenter = Vec3::Zero;
    for (uint32 i = 0; i < 4; ++i) {
      const Vec3 nearCornerView =
          Vec3::Lerp(frustumNearView[i], frustumFarView[i], nearT);
      const Vec3 farCornerView =
          Vec3::Lerp(frustumNearView[i], frustumFarView[i], farT);

      worldCorners[i] = Vec3::Transform(nearCornerView, invView);
      worldCorners[i + 4] = Vec3::Transform(farCornerView, invView);

      frustumCenter += worldCorners[i] + worldCorners[i + 4];
    }
    frustumCenter /= 8.f;

    float radius = 0.f;
    for (const Vec3 &corner : worldCorners)
      radius = max(radius, (corner - frustumCenter).Length());

    radius = max(ceil(radius * 16.f) / 16.f, 1.f);

    const Vec3 up = abs(lightDir.Dot(Vec3::Up)) > 0.99f ? Vec3::Right : Vec3::Up;
    const Vec3 eye = frustumCenter - lightDir * (radius * 2.f);

    Matrix lightView = Matrix::CreateLookAt(eye, frustumCenter, up);

    Vec3 minExtents(FLT_MAX, FLT_MAX, FLT_MAX);
    Vec3 maxExtents(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for (Vec3 &corner : worldCorners) {
      Vec3 cornerLS = Vec3::Transform(corner, lightView);
      minExtents = Vec3::Min(minExtents, cornerLS);
      maxExtents = Vec3::Max(maxExtents, cornerLS);
    }

    const float texelWorldSize =
        max((maxExtents.x - minExtents.x) / shadowMapSize, 1e-5f);
    Vec3 centerLS = (minExtents + maxExtents) * 0.5f;
    centerLS.x = floor(centerLS.x / texelWorldSize) * texelWorldSize;
    centerLS.y = floor(centerLS.y / texelWorldSize) * texelWorldSize;

    const Vec3 halfExtents = (maxExtents - minExtents) * 0.5f;
    minExtents.x = centerLS.x - halfExtents.x;
    maxExtents.x = centerLS.x + halfExtents.x;
    minExtents.y = centerLS.y - halfExtents.y;
    maxExtents.y = centerLS.y + halfExtents.y;

    // [수정] Terrain 높이 범위를 고려한 Z padding 증가
    // Terrain height: (height - 0.5) * 512 → 범위 약 -256 ~ +256
    // 기존 padding으로는 이 높이 범위를 커버하지 못할 수 있음
    const float terrainHeightRange = 512.0f;
    const float zPadding = max(max(radius * 4.f, 50.f), terrainHeightRange);
    const float nearPlane = minExtents.z - zPadding;
    const float farPlane = maxExtents.z + zPadding;
    Matrix lightProj = Matrix::CreateOrthographicOffCenter(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, nearPlane,farPlane);
    passParams.CascadeShadowVP[cascadeIndex] = (lightView * lightProj).Transpose();
  }

  passParams.CascadeSplitDistances =Vec4(CascadeSplit[0], CascadeSplit[1], CascadeSplit[2], CascadeSplit[3]);
}

void RenderSystem::RenderShadow()
{
    mShadowPass->Update(mDeferredDrawBatchs);
}

void RenderSystem::RenderDeferred() {


    mGBufferPass->Update(mDeferredDrawBatchs);
    mLightPass->Update(mLightDrawBatchs);

  // Swapchain OMSet
  int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();



  if (RENDERMANAGER.IsMsaaEnabled()) { // msaa
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitResourceToTarget();
    RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
  } //else {
  //  RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN)).OMSetRenderTargets(1, backIndex);
  //}

  auto& hdrGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::HDR));
  //hdrGroup.WaitResourceToTarget();
  hdrGroup.OMSetRenderTargets();

  RESOURCEMANAGER.Get<Shader>(L"Final")->Update();

  RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

  if (RENDERMANAGER.IsMsaaEnabled()) { // msaa
    RENDERMANAGER.GetRenderTargetGroup( static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();
  }
  hdrGroup.WaitTargetToResource();
}

void RenderSystem::RenderForward() {
    mForwardPass->Update(mDeferredDrawBatchs);
}

void RenderSystem::RenderPost() {
  // m_postStack->update();
    int8 backIndex = RENDERMANAGER.GetSwapChain()->GetBackBufferIndex();

    if (RENDERMANAGER.IsMsaaEnabled()) {
        auto& finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN));
        finalGroup.WaitResourceToTarget();
        finalGroup.OMSetRenderTargets(1, backIndex);
    }
    else {
        auto& finalGroup = RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::SWAP_CHAIN));
        finalGroup.OMSetRenderTargets(1, backIndex);
    }

    RESOURCEMANAGER.Get<Shader>(L"ToneMap")->Update();
    RESOURCEMANAGER.Get<Mesh>(L"Rectangle")->Render();

    if (RENDERMANAGER.IsMsaaEnabled()) {
        RENDERMANAGER.GetRenderTargetGroup(static_cast<uint32>(RENDER_TARGET_GROUP_TYPE::MSAA_SWAP_CHAIN)).WaitTargetToResource();
    }
}

bool RenderSystem::IsFrustumCulled(TransformComponent *trans,
                                   RenderComponent *renderComponent) {
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
