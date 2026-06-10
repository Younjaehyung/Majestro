#include "pch.h"
#include "LobbyRoomBrowserFeature.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "Network.h"
#include "LobbyRoomListComponent.h"
#include "LobbyRoomStateComponent.h"

void LobbyRoomBrowserFeature::Update(float dt)
{
    if (!mWorld) return;

    if (auto em = mWorld->GetEventManager())
    {
        em->Consume<EvRoomError>([this](const EvRoomError& e)
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

LobbyRoomListComponent* LobbyRoomBrowserFeature::GetListComp()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomListComponent>()) return nullptr;
    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomListComponent>();
    if (entities.empty()) return nullptr;
    return mWorld->GetComponent<LobbyRoomListComponent>(entities[0]);
}

LobbyRoomStateComponent* LobbyRoomBrowserFeature::GetStateComp()
{
    if (!mWorld || !mWorld->HasComponentPool<LobbyRoomStateComponent>()) return nullptr;
    auto entities = mWorld->GetEntitiesWithComponent<LobbyRoomStateComponent>();
    if (entities.empty()) return nullptr;
    return mWorld->GetComponent<LobbyRoomStateComponent>(entities[0]);
}

const char* LobbyRoomBrowserFeature::RoomErrorName(uint8 code) const
{
    switch (static_cast<RoomErrorCode>(code))
    {
    case RoomErrorCode::NotHost:           return "Host 가 아닙니다";
    case RoomErrorCode::NotAllReady:       return "전원 Ready 가 아닙니다";
    case RoomErrorCode::NotEnoughPlayers:  return "최소 인원 부족";
    case RoomErrorCode::InvalidRoom:       return "잘못된 방";
    case RoomErrorCode::RoomFull:          return "방이 가득 찼습니다";
    case RoomErrorCode::AlreadyInRoom:     return "이미 다른 방에 있습니다";
    case RoomErrorCode::RoomInGame:        return "게임 중인 방입니다";
    default:                               return "알 수 없음";
    }
}

void LobbyRoomBrowserFeature::PostSpriteRender(std::vector<UIInstanceData>& /*instances*/)
{
#ifdef _IMGUI
    LobbyRoomListComponent* listComp = GetListComp();
    const bool inRoom = (listComp != nullptr) && (listComp->mCurrentRoomId != 0);

    // 홀(방 미입장) : 룸 브라우저 버튼
    if (!inRoom)
    {
        if (!ImGui::Begin("Room Browser"))
        {
            ImGui::End();
            return;
        }

        if (ImGui::Button("Create Room"))
            if (auto em = mWorld->GetEventManager()) em->Enqueue(EvRoomCreate{});
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
            if (auto em = mWorld->GetEventManager()) em->Enqueue(EvRoomListRequest{});

        ImGui::Separator();

        if (listComp == nullptr || !listComp->mHasList)
        {
            ImGui::Text("Loading room list...");
        }
        else if (listComp->mCount == 0)
        {
            ImGui::Text("No rooms. Create one!");
        }
        else
        {
            for (uint8 i = 0; i < listComp->mCount; ++i)
            {
                const LobbyRoomListEntry& e = listComp->mEntries[i];
                ImGui::Text("Room %u   %u / %u   %s",
                    e.roomId,
                    static_cast<unsigned>(e.playerCount),
                    static_cast<unsigned>(e.maxPlayers),
                    e.phase == 1 ? "[InGame]" : "[Waiting]");
                ImGui::SameLine();

                ImGui::PushID(static_cast<int>(e.roomId));
                if (ImGui::Button("Join"))
                    if (auto em = mWorld->GetEventManager()) em->Enqueue(EvRoomJoin{ e.roomId });
                ImGui::PopID();
            }
        }

        if (mLastErrorCode != 0)
        {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", RoomErrorName(mLastErrorCode));
        }

        ImGui::End();
        return;
    }

    // 대기실(방 안) : 방 나가기 버튼
    if (!ImGui::Begin("Lobby Room"))
    {
        ImGui::End();
        return;
    }

    ImGui::Text("Room %u  (R: Ready, G: Start)", listComp->mCurrentRoomId);

    if (ImGui::Button("Leave Room"))
    {
        const uint32 rid = listComp->mCurrentRoomId;
        if (auto em = mWorld->GetEventManager()) em->Enqueue(EvRoomLeave{ rid });

        listComp->mCurrentRoomId = 0;
        if (auto* st = GetStateComp()) st->mHasSnapshot = false;
    }

    if (mLastErrorCode != 0)
    {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Error: %s", RoomErrorName(mLastErrorCode));
    }

    ImGui::End();
#endif
}
