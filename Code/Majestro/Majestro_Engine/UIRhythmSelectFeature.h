#pragma once
#include "UIFeature.h"

class UIRhythmSelectFeature : public UIFeature
{
public:
    void Initialize(World* world) override;
    void Update(float dt) override;

private:
    static constexpr int32 kRowCount = 3;   // 부모 리듬
    static constexpr int32 kColCount = 3;   // 원본/파생1/파생2

    void Open();
    void Close();
    void SetVisible(bool visible);
    void ApplyCharacterAssets();            // 로컬 캐릭터의 Line/Portrait/Sheet 로 교체
    void ApplyDiscRects();                  // 디스크 3장 소스 렉트 갱신
    void ResetDiscVisuals();                // 회전/스케일/알파 원복

    // 레코드판 스핀 교체 연출
    void StartDiscSpin(int32 subIndex);
    void UpdateDiscSpin(float dt);

    // 열림 연출
    void StartIntro();
    void UpdateIntro(float dt);
    void FinishIntro();
    void EnableButtons(bool enabled);
    void UpdateVisualizer(float dt);        // 링 위치/크기 추적 + 웨이브 펄스 주입
    void UpdateSaveFlash(float dt);
    void UpdateBgmFade(float dt);           // 광장 BGM(Ambient) 페이드 (미리듣기 전환)

    void OnArrowClicked(int32 direction);   // 포커스 행의 열 전환 (+1/-1)
    void OnSubDiscClicked(int32 subIndex);  // 행 포커스 교체
    void OnPlayClicked();                   // 미리듣기 토글
    void OnSaveClicked();                   // 로컬 선택 저장

    int32 GetSubRow(int32 subIndex) const;  // 서브 디스크가 표시하는 행
    void StartPreview();
    void StopPreview();
    void ApplyPreviewParams();              // 재생 중 파라미터 즉시 갱신
    void RefreshPlayButtonTint();

    // 엔티티
    Entity mBackground{};       // 공용 배경 (RHYTHM 타이틀)
    Entity mLine{};             // 캐릭터 테마색 스플래시
    Entity mPortrait{};         // 캐릭터 초상 (좌측)
    Entity mDiscMain{};         // 메인 디스크 (포커스 행 x 선택 열)
    Entity mDiscSubs[2]{};      // 나머지 행 디스크 (클릭 시 포커스 교체)
    Entity mArrowLeft{};
    Entity mArrowRight{};
    Entity mBackButton{};
    Entity mPlayButton{};
    Entity mSaveButton{};       // [임시] Save 전용 이미지 제작 전까지 Play 텍스트 + 골드 틴트
    Entity mVisualizer{};       // CircularVisualizerComponent (메인 디스크 링)

    // 선택 상태
    int32 mColumns[kRowCount] = { 0, 0, 0 };  // 행별 선택 열 (RhythmSubVariant)
    int32 mFocusRow = 0;                      // 메인 디스크로 보는 행 (0=R1)
    int32 mSubRows[2] = { 1, 2 };             // 서브 디스크가 표시하는 행 (스왑 시 자리 유지)
	uint8 mPlayerType = 0;                    // 로컬 캐릭터

    bool mVisible = false;

    // 디스크 스핀 연출
    bool  mSpinActive = false;
    bool  mSpinRectApplied = false;      // 스핀 중간 지점에서 렉트 교체 완료 여부
    bool  mSpinTargets[3] = {};          // 회전 대상 (0=메인, 1=서브0, 2=서브1)
    float mSpinTime = 0.f;

    // 비주얼라이저 웨이브 펄스
    float mPulseTime = -1.f;         // < 0 이면 비활성

    // 열림 연출
    float mIntroTime = -1.f;         // < 0 이면 비활성. 진행 중에는 조작 입력 X
    bool  mIntroPulseFired = false;  // 디스크 등장 시점의 링 펄스 1회 발화 여부

    // 미리듣기 / 광장 BGM 페이드
    bool  mPreviewPlaying = false;
    float mAmbientVolume = 1.f;
    float mAmbientVolumeTarget = 1.f;

    // Save 확인 팝 연출
    float mSaveFlashTime = -1.f;     // < 0 이면 비활성
};
