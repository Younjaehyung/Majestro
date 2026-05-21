#include "pch.h"
#include "LobbyRoomUIFeature.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"
#include "LobbyRoomStateComponent.h"

void LobbyRoomUIFeature::Initialize(World* world)
{
    UIFeature::Initialize(world);
}

void LobbyRoomUIFeature::Update(float dt)
{
    // RoomError 이벤트 소비 — Render 콜백에서는 ImGui 만 다루도록 분리
    if (auto eventMgr = mWorld->GetEventManager())
    {
        eventMgr->Consume<EvRoomError>([this](const EvRoomError& e)
        {
            mLastErrorCode = e.errorCode;
            mErrorRemain = 3.0f;
        });
    }

    if (mErrorRemain > 0.f)
    {
        mErrorRemain -= dt;
        if (mErrorRemain <= 0.f) mLastErrorCode = 0;
    }
}

void LobbyRoomUIFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{
#ifdef _IMGUI
    if (!mWorld->HasComponentPool<LobbyRoomStateComponent>())
        return;

    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
    if (entities.empty()) return;

    LobbyRoomStateComponent* state = mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]);
    if (!state) return;

    const uint32 myClientId = Network::GetInstance().mClientId;

    if (!ImGui::Begin("Lobby Room"))
    {
        ImGui::End();
        return;
    }

    if (!state->mHasSnapshot)
    {
        ImGui::Text("Waiting for room state...");
        ImGui::End();
        return;
    }

    ImGui::Text("Room %u   %u / %u",
        state->mRoomId,
        static_cast<unsigned>(state->mPlayerCount),
        static_cast<unsigned>(state->mMaxPlayers));
    ImGui::Separator();

    bool isMeHost = false;
    bool allReady = (state->mPlayerCount > 0);

    for (uint8 i = 0; i < state->mPlayerCount && i < 8; ++i)
    {
        const auto& slot = state->mSlots[i];
        const bool isMe = (slot.sessionId == myClientId);
        if (isMe && slot.isHost) isMeHost = true;
        if (!slot.ready) allReady = false;

        // 본인 슬롯은 색상 강조
        if (isMe)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.4f, 1.0f));

        ImGui::Text("%s Player %u   %-8s   Ready: %s%s",
            slot.isHost ? "[Host]" : "      ",
            slot.sessionId,
            GetPlayerTypeName(slot.playerType),
            slot.ready ? "O" : "X",
            isMe ? "  <- You" : "");

        if (isMe)
            ImGui::PopStyleColor();
    }

    ImGui::Separator();
    ImGui::Text("R : Ready 토글");
    if (isMeHost)
    {
        if (allReady) ImGui::Text("G : Start Game");
        else ImGui::TextDisabled("G : Start Game (모든 플레이어 Ready 필요)");
    }
    else
    {
        ImGui::TextDisabled("G : Host 전용");
    }

    if (mLastErrorCode != 0)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
        ImGui::Text("거부: %s", GetRoomErrorName(mLastErrorCode));
        ImGui::PopStyleColor();
    }

    ImGui::End();
#endif
}

const char* LobbyRoomUIFeature::GetPlayerTypeName(uint8 playerType) const
{
    switch (playerType)
    {
    case 0: return "Rudwig";
    case 1: return "Ibanix";
    case 2: return "Fanthor";
    default: return "Unknown";
    }
}

const char* LobbyRoomUIFeature::GetRoomErrorName(uint8 code) const
{
    switch (static_cast<RoomErrorCode>(code))
    {
    case RoomErrorCode::NotHost:           return "Host 가 아닙니다";
    case RoomErrorCode::NotAllReady:       return "전원 Ready 가 아닙니다";
    case RoomErrorCode::NotEnoughPlayers:  return "최소 인원 부족";
    case RoomErrorCode::InvalidRoom:       return "잘못된 방";
    default:                               return "알 수 없음";
    }
}
