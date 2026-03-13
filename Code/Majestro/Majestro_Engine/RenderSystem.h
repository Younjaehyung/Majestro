#pragma once
#include "Buffer.h"
#include "ComponentPool.h"
#include "LightComponent.h"
#include "RenderComponent.h"
#include "Shader.h"
#include "System.h"
#include "TransformComponent.h"
#include "World.h"
#include "NavMeshLoader.h"

struct MaterialParams;
struct PatricleParams;

class RenderPass;

class Mesh;
class Shader;
class Material;

class CameraComponent;

struct RenderParams {
  uint32 object0{}; // object index
  uint32 object1{}; // material index

  int32 object2{}; //// animation
  uint32 object3{};
};

struct PassParams {

  Matrix MatView;
  Matrix MatProjection;
  Matrix MatViewInv;       // view의 역행렬
  Matrix MatProjectionInv; // Projection의 역행렬	(사용은 선택)

  Vec2 ScreenSize{};
  Vec2 MinMaxTessDistance;

  Vec2 HeightMapResolution;
  float MaxTessLevel;
  float TotalTime;

  uint32 TileCountX;
  uint32 TileCountZ;
  uint32 LightsCount{};
  uint32 SkyBoxIndex{};

  int32 TerrainSlot1;
  int32 TerrainSlot2;
  int32 TerrainSlot3;
  int32 TerrainSlot4;
  int32 TerrainSlot5;
  int32 TerrainSlot6;

  int32 Padding0;
  int32 Padding1;

  Vec4 CascadeSplitDistances;
  array<Matrix, 4> CascadeShadowVP{};
};

// 디버그 라인 렌더링 요청 구조체
// NavMeshDebugRenderer::DrawLine 등에서 RenderSystem::SubmitDebugLine()으로 추가
struct DebugLineRequest {
  Vec3 start;
  Vec3 end;
  Vec4 color; // (1,0,0,1)=빨강, (0,1,0,1)=초록, 그 외=흰색
};

struct DrawItem {
  shared_ptr<Shader> PSOShader{};
  shared_ptr<Mesh> PMesh{};

  uint32 PSOID{};
  uint32 MeshID{};
  uint32 SubMesh{};
  uint32 SubMeshIndex{};

  RenderParams InstanceGPU;
  DrawItem() = default;
  DrawItem(shared_ptr<Shader> &shader, shared_ptr<Mesh> &mesh, uint32 psoID,
           uint32 meshID, uint32 subMesh, uint32 subMeshIndex,
           RenderParams instanceGPU) {
    PSOShader = shader;
    PMesh = mesh;
    PSOID = psoID;
    MeshID = meshID;
    SubMesh = subMesh;
    SubMeshIndex = subMeshIndex;
    InstanceGPU = instanceGPU;
  }
};

struct DrawBatch {
  shared_ptr<Shader> PSOShader{};
  shared_ptr<Mesh> Mesh{};

  uint32 PSOID{};
  // uint32 MeshID{};
  // uint32 SubMesh{};
  uint32 SubMeshIndex{};

  uint32 BaseInstance{};
  uint32 InstanceCount{};

  DrawBatch() = default;
  DrawBatch(DrawItem *drawItem) {
    PSOShader = drawItem->PSOShader;
    Mesh = drawItem->PMesh;
    PSOID = drawItem->PSOID;
    // MeshID = drawItem->MeshID;
    // SubMesh = drawItem->SubMesh;
    SubMeshIndex = drawItem->SubMeshIndex;
  }
};

class RenderSystem : public System {
public:
  RenderSystem(World *world);

  void Initialize();
  void Update();

  // 디버그 라인 제출 (어느 시스템에서나 호출 가능, RenderSystem이 프레임 내 소비)
  static void SubmitDebugLine(const Vec3& start, const Vec3& end, const Vec4& color);

private:           // RenderPass
  void ClearRTV(); // clear
  void ClearBuffer();
  void PushData();
  void PreProcess();
  void RenderPass();
  
private: // Culling
  bool IsCustomCulled(uint8 layer) {
    return (mCullingMask & (1 << layer)) != 0;
  }
  bool IsFrustumCulled(TransformComponent *trans,
                       RenderComponent *renderComponent);

private: // Push&Clear Data
  void PushMaterialData();
  void PushCubeData();
  void PushLandData();
  void PushPassData();
  void PushFrameData();
  void PushShadowCascades();       // cascade VP 행렬 + 라이트 구체 사전 계산
  void PushInstanceData();
  void PushObjectData();
  void PushLightData();
  void PushDebugging();

  void UpdateCascadeShadowMatrices(LightComponent* lightComponent);

private: // Render
  void InstancingRender(DrawBatch &);

  // depth prepass
  void RenderDepthPrePass();

  // shadow
  void RenderShadow();

  // deferred
  void RenderDeferred(); // 2pass //clear

  // forward
  void RenderForward();

  // 인게임 VFX (HDR RT, ToneMap 전)
  void RenderEffect();

  // post process
  void RenderPost();



private:
  uint32 mFrameCount = 0;
  NavigationSystem     mNavSystem;
  NavMeshDebugRenderer mNavMeshDebugRenderer;
  Entity mCameraID;
  CameraComponent *mCamera{};
  uint32 mCullingMask = 0;

  shared_ptr<RootSignature> mRootSignature;
  ComponentPool<RenderComponent> *mRenderComponentPool = nullptr;

private: // 배치 버퍼
  // Pass별 PSO에 따른 분류
  std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)> mPassTable{};

  std::vector<DrawItem> mDeferredDrawItems;
  std::vector<DrawItem> mShadowOnlyDrawItems; // 카메라 밖 + 라이트 프러스텀 안

  std::vector<DrawBatch> mDeferredDrawBatchs;
  std::vector<DrawBatch> mShadowOnlyBatchs;   // shadow pass 전용 배치
  std::vector<DrawBatch> mLightDrawBatchs;

  struct dummy {
    uint32 BaseInstance;
    uint32 InstanceCount;
    uint32 Cascade;
  } dum;

private:
  std::vector<Entity> mDummyVector;
  // RenderManager의 structuerdBuffer로 복사할 데이터들

  std::vector<RenderParams> mInstanceVector;
  std::vector<LightParams> mLightVector;
  std::vector<ObjectParams> mObjectVector;
  std::vector<MaterialParams> mMaterialVector;
  std::vector<PatricleParams> mPatricleVector;

  array<bool, 4> mCascadeActive = { true, true, true, true };
  array<float, 4> CascadeSplit = { 0.f, 0.f, 0.f, 0.f };
  float mCascadeSplitLambda = 0.8f;
  array<Matrix, 4> mCascadeView{};
  array<Matrix, 4> mCascadeProjection{};

  // 라이트 프러스텀 구체 (카메라 밖 오브젝트 shadow 컬링용)
  array<Vec3, 4> mCascadeFrustumCenter{};
  array<float, 4> mCascadeFrustumRadius{};

private:
  // 변수 재사용을 막기 위해 둔 Dummy Parms
  PassParams passParams{};
  ObjectParams objectParams{};
  LightParams lightParams{};
  RenderParams renderParams{};

  DrawBatch mBatch{};
  uint32 mCurrPSOID{};

private: // 디버그용 충돌박스 / 라인
  shared_ptr<Mesh> mWireCube;
  shared_ptr<Mesh> mLineMesh;                // 단위 선분 메쉬 (0,0,0)→(1,0,0)
  shared_ptr<Material> mDebugLineMat;        // 기본 (Depth Test O)
  shared_ptr<Material> mDebugLineNoDepthMat; // 항상 보임 (Depth Test X)
  shared_ptr<Material> mDebugLineGreenMat;   // 초록
  shared_ptr<Material> mDebugLineRedMat;     // 빨강
  bool mDrawColliders = false;

  static std::vector<DebugLineRequest> sDebugLineQueue; // 프레임당 디버그 라인 큐

private: // RenderPass
	std::shared_ptr<class DepthPrePass>   mDepthPrePass;
	std::shared_ptr<class ShadowPass>     mShadowPass;
	std::shared_ptr<class GBufferPass>    mGBufferPass;
	std::shared_ptr<class LightsPass>     mLightPass;
	std::shared_ptr<class ForwardPass>    mForwardPass;
	std::shared_ptr<class EffectPass>     mEffectPass;
	std::shared_ptr<class PostProcessPass> mPostProcessPass;


};
