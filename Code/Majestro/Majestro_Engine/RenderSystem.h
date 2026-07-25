#pragma once
#include "Buffer.h"
#include "ComponentPool.h"
#include "RenderPass.h"
#include "LightComponent.h"
#include "RenderComponent.h"
#include "Shader.h"
#include "System.h"
#include "TransformComponent.h"
#include "World.h"
#include "NavMeshLoader.h"

struct MaterialParams;
struct ParticleSharedParams;


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

struct ObjectParams {
  Matrix MatWorld;

  // Extra1.x = ObjectAlpha (1=불투명, 0=완전 디더 컬)
  // Extra1.y = HitFlashStrength (0~1, JHToon_PS에서 red lerp 가중치)
  // Extra1.z = EmissiveGate, Extra1.w = DissolveAmount
  Vec4 Extra1{ 1.f, 0.f, 0.f, 0.f };

  // Extra2 = 대상 강조(나노강화) : rgb = 강조 색(HDR 가산), w = 강도(0=미강조)
  Vec4 Extra2{ 0.f, 0.f, 0.f, 0.f };
};

struct PassParams {

  Matrix MatPrevView;
  Matrix MatPrevProjection;
  Matrix MatPrevViewInv;       // view의 역행렬
  Matrix MatPrevProjectionInv; // Projection의 역행렬	(사용은 선택)

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

  int32 IrradianceIndex{-1};     // CubeBoxMaps 배열 인덱스 (IBL Diffuse)
  int32 PreFilteredEnvIndex{-1}; // CubeBoxMaps 배열 인덱스 (IBL Specular)

  Vec4 CascadeSplitDistances;
  array<Matrix, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> CascadeShadowVP{};

  // IBL 추가 파라미터 (구조체 끝, HLSL과 레이아웃 동일)
  int32 BrdfLutIndex{-1};        // TextureMaps 배열 인덱스 (BRDF LUT 2D)
  int32 PreFilteredMipLevels{8}; // pre-filtered 큐브맵 mip 단계 수
  int32 IBLPadding0{};
  int32 IBLPadding1{};

  // 프로젝트는 방향광 하나만 사용한다.
  // 픽셀마다 StructuredBuffer를 조회하지 않도록 패스 상수에 직접 저장한다.
  LightParams DirectionalLight{};
};

// 디버그 라인 렌더링 요청 구조체
// NavMeshDebugRenderer::DrawLine 등에서 RenderSystem::SubmitDebugLine()으로 추가
struct DebugLineRequest {
  Vec3 start;
  Vec3 end;
  Vec4 color; // (1,0,0,1)=빨강, (0,1,0,1)=초록, (1,1,0,1)=노랑, 그 외=흰색
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
  static void SetDrawColliders(bool enabled);
  static bool GetDrawColliders();
  static void SetDrawCullingOBB(bool enabled);
  static bool GetDrawCullingOBB();
  static void SetDrawEnemyRanges(bool enabled);
  static bool GetDrawEnemyRanges();
  static void SetDrawEnemyAttackRanges(bool enabled);
  static bool GetDrawEnemyAttackRanges();
  static void SetDrawPlayerAttackRanges(bool enabled);
  static bool GetDrawPlayerAttackRanges();

  // 파이프라인 교체
  // 씬 Initialize()에서 호출 — pipeline->Initialize(mWorld)
  void SetPipeline(shared_ptr<IRenderPipeline> pipeline);
  shared_ptr<IRenderPipeline> GetPipeline() const { return mActivePipeline; }

public:
    // ImGui 튜닝용 — CSM 스플릿 분포 lambda (0=균등, 1=로그)
    float* GetCascadeSplitLambdaPtr() { return &mCascadeSplitLambda; }

    // ImGui 디버그 — 맵 고정 cascade 상태
    bool IsMapCascadeDirty() const { return mMapCascadeDirty; }
    uint64 GetMapCascadeRedrawTotal() const;
    uint32 GetStaticCasterCount() const { return mPrevStaticCasterCount; }
    void ForceMapCascadeDirty() { InvalidateMapCascadeBounds(); }

    // ImGui 디버그 — cascade별 split 거리와 텍셀 크기
    const array<float, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>& GetCascadeSplits() const { return CascadeSplit; }
    float GetCascadeTexelWorldSize(uint32 cascadeIndex) const {
        if (cascadeIndex >= RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT || shadowMapSize <= 0.f) return 0.f;
        return (mCascadeFrustumRadius[cascadeIndex] * 2.f) / shadowMapSize;
    }
    bool IsCascadeActive(uint32 cascadeIndex) const {
        return cascadeIndex < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT && mCascadeActive[cascadeIndex];
    }


private:           // RenderPass
  void ClearRTV(); // clear
  void ClearBuffer();
  void PushData();
  int b = false;
private: // Culling
  bool IsCustomCulled(uint8 layer) {
    return (mCullingMask & (1 << layer)) != 0;
  }
  bool IsVisibleInFrustum(TransformComponent *trans,
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
  void UpdateMapCascade(const Vec3& lightDir);  // 맵 전체 고정 cascade (마지막 슬라이스)
  void InvalidateMapCascadeBounds();
  void InvalidateMapCascadeContent();
  // 오브젝트 구체가 교차하는 cascade 비트마스크 — 4-cascade는 정적 캐스터 + dirty 프레임 한정
  uint8 ComputeCascadeMask(const Vec3& objCenter, float objRadius, bool isStatic) const;

private: // Render
  void InstancingRender(DrawBatch &);

private:
  uint32 mFrameCount = 0;
  NavigationSystem     mNavSystem;
  NavMeshDebugRenderer mNavMeshDebugRenderer;
  Entity mCameraID;
  CameraComponent *mCamera{};
  uint32 mCullingMask = 0;

  shared_ptr<IRenderPipeline> mActivePipeline;  // 현재 활성 파이프라인

  shared_ptr<RootSignature> mRootSignature;
  ComponentPool<RenderComponent> *mRenderComponentPool = nullptr;

private: // 배치 버퍼
  // Pass별 PSO에 따른 분류
  std::array<PassCustomData, static_cast<uint32>(PASS_CUSTOM_INDEX::PASS_CUSTOM_COUNT)> mPassTable{};

  std::vector<DrawItem> mDeferredDrawItems;


  std::array<std::vector<DrawItem>, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeDrawItems{};
  std::array<std::vector<DrawBatch>, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeDrawBatchs{};

  std::vector<DrawBatch> mDeferredDrawBatchs;
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
  std::vector<ObjectParams> mObjectVector;
  std::vector<MaterialParams> mMaterialVector;
  std::vector<ParticleSharedParams> mPatricleVector;

  array<bool, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeActive = { true, true, true, true };
  array<float, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> CascadeSplit = { 0.f, 0.f, 0.f, 0.f };
  array<Matrix, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeView{};
  array<Matrix, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeProjection{};
  float shadowMapSize = static_cast<float>(SHADOW_MAP_RESOLUTION);
  float mCascadeSplitLambda = 0.6f;

  // 라이트 프러스텀 구체 (shadow 컬링용)
  array<Vec3, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT>  mCascadeFrustumCenter{};
  array<float, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeFrustumRadius{};


  array<Vec3, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeLightRight{};
  array<Vec3, RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT> mCascadeLightUp{};
  bool mHasDirectionalShadow = false;


  bool   mMapCascadeDirty = true;                // 현재 프레임에 고정 그림자 슬라이스를 다시 그릴지 여부
  Matrix mMapCascadeVP = Matrix::Identity;       // 고정 VP (transpose 전)
  Vec3   mPrevLightDir = Vec3::Zero;             // bounds dirty 검출: 라이트 방향 변화
  uint32 mPrevStaticCasterCount = 0;             // bounds dirty 검출: 잠재 정적 캐스터 수 변화
  uint64 mStaticCasterBoundsRevision = 1;        // mesh, static 상태, transform 경계 변경 세대
  uint64 mAppliedStaticCasterBoundsRevision = 0; // 맵 cascade VP에 반영된 bounds 세대
  uint64 mStaticCasterContentRevision = 1;       // visibility 등 그림자 내용 변경 세대
  uint64 mAppliedStaticCasterContentRevision = 0;// 고정 그림자 슬라이스에 반영된 내용 세대
  uint64 mMapCascadeRedrawTotal = 0;             // ImGui 진단용 누적 재렌더 횟수 (리셋 없음)

  // 정적 캐스터 전체 월드 AABB
  Vec3 mStaticCasterAabbMin = Vec3::Zero;
  Vec3 mStaticCasterAabbMax = Vec3::Zero;
  bool mHasStaticCasterBounds = false;

private:
  // 변수 재사용을 막기 위해 둔 Dummy Parms
  PassParams passParams{};
  ObjectParams objectParams{};
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
  shared_ptr<Material> mDebugLineYellowMat;  // 노랑
  static bool sDrawColliders;
  static bool sDrawCullingOBB;   // 프러스텀 컬링에 쓰이는 mWorldOBB 시각화
  static bool sDrawEnemyRanges;
  static bool sDrawEnemyAttackRanges;
  static bool sDrawPlayerAttackRanges;

  static std::vector<DebugLineRequest> sDebugLineQueue; // 프레임당 디버그 라인 큐

};
