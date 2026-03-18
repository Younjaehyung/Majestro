#include "pch.h"
#include "NetTransformComponent.h"

void NetTransformComponent::OnSnapshot(const NetSnapshot& s, double localRecvSec)
{
    if (!mHasAnchor || IsNewer(s.serverTick, mAnchorServerTick))
    {
        mAnchorServerTick = s.serverTick;
        mAnchorLocalSec = localRecvSec;
        mHasAnchor = true;
    }

    // tick 기준 정렬 삽입
    InsertSorted(s);

    // 너무 오래된 스냅샷 제거 (tick 기반)
    PruneOld(2.0); // 2초 보관
}

void NetTransformComponent::UpdateRender(double localNowSec)
{
    if (!mHasAnchor || mBuffer.empty())
        return;

    const double estimatedNowTickF =
        (double)mAnchorServerTick +
        (localNowSec - mAnchorLocalSec) * mServerHz +
        mLatencyLeadTicksF; 

    const double renderTickF = estimatedNowTickF - mInterpDelayTicksF;

    // renderTickF를 감싸는 A,B 찾기
    NetSnapshot A, B;
    if (FindBracketing(renderTickF, A, B))
    {
        const uint32_t spanTicksU = (B.serverTick - A.serverTick);
        const double   spanTicks = (double)spanTicksU;

        const double t01 = (spanTicks > 1e-6) ? (renderTickF - (double)A.serverTick) / spanTicks : 0.0;
        const float  t = (float)std::clamp(t01, 0.0, 1.0);


        const float dtABSec = (float)(spanTicks * mFixedDtSec);

        // 위치: Hermite 보간(속도 사용)
        mRenderPos = Hermite(A.pos, A.vel * dtABSec, B.pos, B.vel * dtABSec, t);

        // 회전
        mRenderRotQ = Quaternion::Slerp(A.rotQ, B.rotQ, t);
        return;
    }

    // 못 찾으면 제한적 외삽 (tick 기반)
    const NetSnapshot& L = mBuffer.back();
    const double exTicksF = renderTickF - (double)L.serverTick;

    if (exTicksF >= 0.0 && exTicksF <= mMaxExtrapolationTicksF)
    {
        const float exSec = (float)(exTicksF * mFixedDtSec);
        // 외삽이 길어질수록 속도를 점진적으로 줄여 패킷 손실 시 날아가는 현상 방지
        const float damping = max(0.0f, 1.0f - (float)(exTicksF / mMaxExtrapolationTicksF));
        mRenderPos = L.pos + L.vel * (exSec * damping);
        mRenderRotQ = L.rotQ;
    }
    else
    {
        // 외삽 한계 초과: 마지막 수신 위치에서 홀드
        mRenderPos = L.pos;
        mRenderRotQ = L.rotQ;
    }
}


void NetTransformComponent::SetServerHz(double serverHz)
{
    mServerHz = max(serverHz, 1.0);
    mFixedDtSec = 1.0 / mServerHz;

   
    RecomputeTickParams();
}

// 스냅샷 전송 Hz(서버->클라 상태) 설정
void NetTransformComponent::SetSnapshotRateHz(double snapshotHz)
{
    mSnapshotHz = max(snapshotHz, 1.0);
    RecomputeTickParams();
}

// (선택) RTT/2 기반 lead tick(한 방향 지연 근사)을 넣고 싶으면 사용
// 예: rttSec * 0.5 * serverHz
void NetTransformComponent::SetLatencyLeadTicks(double leadTicks)
{
    mLatencyLeadTicksF = max(leadTicks, 0.0);
}



bool NetTransformComponent::IsNewer(uint32_t a, uint32_t b)
{
    return (int32_t)(a - b) > 0;
}

Vec3 NetTransformComponent::Hermite(const Vec3& p0, const Vec3& m0,
    const Vec3& p1, const Vec3& m1, float t)
{
    const float t2 = t * t;
    const float t3 = t2 * t;
    const float h00 = 2 * t3 - 3 * t2 + 1;
    const float h10 = t3 - 2 * t2 + t;
    const float h01 = -2 * t3 + 3 * t2;
    const float h11 = t3 - t2;
    return p0 * h00 + m0 * h10 + p1 * h01 + m1 * h11;
}

void NetTransformComponent::InsertSorted(const NetSnapshot& s)
{
    // tick 기준 정렬 삽입
    if (mBuffer.empty() || s.serverTick >= mBuffer.back().serverTick)
    {
        mBuffer.push_back(s);
        return;
    }


    for (size_t i = mBuffer.size(); i-- > 0; )
    {
        if (mBuffer[i].serverTick <= s.serverTick)
        {
            mBuffer.insert(mBuffer.begin() + (i + 1), s);
            return;
        }
    }
    // 모두보다 작으면 맨 앞
    mBuffer.insert(mBuffer.begin(), s);
}

bool NetTransformComponent::FindBracketing(double renderTickF, NetSnapshot& outA, NetSnapshot& outB) const
{
    if (mBuffer.size() < 2) return false;

    // tick 범위 체크
    if (renderTickF < (double)mBuffer.front().serverTick) return false;
    if (renderTickF > (double)mBuffer.back().serverTick)  return false;

    for (size_t i = 0; i + 1 < mBuffer.size(); ++i)
    {
        const auto& A = mBuffer[i];
        const auto& B = mBuffer[i + 1];

        const double a = (double)A.serverTick;
        const double b = (double)B.serverTick;

        if (a <= renderTickF && renderTickF <= b)
        {
            outA = A; outB = B;
            return true;
        }
    }
    return false;
}

void NetTransformComponent::PruneOld(double keepSec)
{
    if (mBuffer.empty()) return;

    const uint32_t keepTicks = (uint32_t)max(1.0, keepSec * mServerHz);
    const uint32_t newest = mBuffer.back().serverTick;
    const uint32_t minTick = (newest > keepTicks) ? (newest - keepTicks) : 0;

    while (mBuffer.size() > 2 && mBuffer.front().serverTick < minTick)
        mBuffer.erase(mBuffer.begin());
}

void NetTransformComponent::RecomputeTickParams()
{
    // 스냅샷 간격(초)
    const double snapshotIntervalSec = 1.0 / mSnapshotHz;


    const double interpDelaySec = max(0.10, 2.5 * snapshotIntervalSec);

    // 모두 tick로 환산해 저장
    mInterpDelayTicksF = interpDelaySec * mServerHz;
    mMaxExtrapolationTicksF = mMaxExtrapolationSec * mServerHz;
}
