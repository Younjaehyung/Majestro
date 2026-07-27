#include "pch.h"
#include "IMGUISystem.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "RenderManager.h"
#include "RenderSystem.h"
#include "AudioVisualizerComponent.h"
#include "CameraComponent.h"
#include "LightComponent.h"
#include "SystemManager.h"
#include "WeaponTrailComponent.h"
#include "SocketComponent.h"
#include "TagComponent.h"
#include "TransformComponent.h"
#include "LobbyRoomSystem.h"
#include "UITransformComponent.h"
#include "UISpriteComponent.h"

IMGUIRenderSystem::IMGUIRenderSystem(World* world) : System::System(world)
{
    mPhase = SysPhase::Render;
	mOrder = 100;  // 다른 Sim 시스템 이후에 실행
}

void IMGUIRenderSystem::Initialize()
{
}

void IMGUIRenderSystem::Update()
{
#ifdef _IMGUI
    ImGui::Begin("Inspector");

    if (mWorld->HasComponentPool<IMGUIComponent>())
    {
        std::vector<Entity> entitys{ mWorld->GetEntitiesWithComponent<IMGUIComponent>() };

        for (auto& entity : entitys)
        {
            IMGUIComponent* comp = mWorld->GetComponent<IMGUIComponent>(entity);
            if (!comp) continue;

            if (ImGui::CollapsingHeader(comp->GetName()))
            {
               

                for (auto& prop : comp->mProps)
                    DrawProperty(prop);
            }
        }
    }


    ImGui::End();

    if (auto* renderSys = mWorld->GetSystemManager()->GetSystem<RenderSystem>())
    {
        if (auto pipeline = renderSys->GetPipeline())
            pipeline->DrawImGui();

        // CSM 스플릿 분포 lambda
        ImGui::SliderFloat("CSM Split Lambda", renderSys->GetCascadeSplitLambdaPtr(), 0.0f, 1.0f);

        // cascade별 split 거리 / 텍셀 크기
        // 텍셀이 클수록 그림자가 번짐
        const auto& splits = renderSys->GetCascadeSplits();
        for (uint32 ci = 0; ci < RENDER_TARGET_SHADOW_GROUP_MEMBER_COUNT; ++ci)
        {
            const bool isMap = (ci == SHADOW_MAP_CASCADE_INDEX);
            ImGui::Text("Cascade %u %-5s split %8.1f  texel %6.2f  %s",
                ci, isMap ? "(map)" : "", splits[ci],
                renderSys->GetCascadeTexelWorldSize(ci),
                renderSys->IsCascadeActive(ci) ? "" : "(off)");
        }

        // 맵 고정 cascade
        ImGui::Text("Map Cascade  redraws: %llu  staticCasters: %u",
            static_cast<unsigned long long>(renderSys->GetMapCascadeRedrawTotal()),
            renderSys->GetStaticCasterCount());
        ImGui::SameLine();
        if (ImGui::Button("Rebuild##MapCascade"))
            renderSys->ForceMapCascadeDirty();
    }

    // ── Camera Inspector ──────────────────────────────────────────────────────
    DrawCameraInspector();

    // ── Light Inspector ───────────────────────────────────────────────────────
    DrawLightInspector();

    // ── Weapon Trail Inspector ─────────────────────────────────────────────────
    DrawWeaponTrailInspector();

    // Lobby character transform inspector change
    DrawLobbyCharacterTransformInspector();

    // 로비 룸 UI(info/ready/choose) 아틀라스 슬라이스/배치 조정
    DrawLobbyRoomUIInspector();
#endif
}

void IMGUIRenderSystem::DrawLobbyCharacterTransformInspector()
{
#ifdef _IMGUI
    if (mWorld->GetSceneId() != SceneId::Lobby)
        return;

    if (!ImGui::Begin("Lobby Character Transform"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<MannequinComponent>() ||
        !mWorld->HasComponentPool<TransformComponent>())
    {
        ImGui::Text("Lobby mannequin character not found");
        ImGui::End();
        return;
    }

    static const char* characterNames[] =
    {
        "Rudwig",
        "Ibanix",
        "Fanthor"
    };
    constexpr uint8 characterNameCount = 3;

    auto entities = mWorld->GetEntitiesWithComponent<MannequinComponent>();
    for (auto e : entities)
    {
        MannequinComponent* mannequin = mWorld->GetComponent<MannequinComponent>(e);
        TransformComponent* transform = mWorld->GetComponent<TransformComponent>(e);
        if (!mannequin || !transform)
            continue;

        const uint8 playerType = mannequin->mPlayerType;
        const char* characterName =
            playerType < characterNameCount
            ? characterNames[playerType]
            : "Unknown";

        char header[96];
        snprintf(header, sizeof(header), "%s Character (Entity %u)", characterName, e.GetID());

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(static_cast<int>(e.GetID()));

            ImGui::DragFloat3("Position", &transform->mLocalPosition.x, 1.0f, -10000.0f, 10000.0f, "%.2f");
            ImGui::DragFloat3("Rotation", &transform->mLocalRotationE.x, 1.0f, -360.0f, 360.0f, "%.2f");

            ImGui::Text("Current Position  %.2f  %.2f  %.2f",
                transform->mLocalPosition.x,
                transform->mLocalPosition.y,
                transform->mLocalPosition.z);
            ImGui::Text("Current Rotation  %.2f  %.2f  %.2f",
                transform->mLocalRotationE.x,
                transform->mLocalRotationE.y,
                transform->mLocalRotationE.z);

            ImGui::PopID();
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawLobbyRoomUIInspector()
{
#ifdef _IMGUI
    if (mWorld->GetSceneId() != SceneId::Lobby)
        return;

    auto* lobby = mWorld->GetSystemManager()->GetSystem<LobbyRoomSystem>();
    if (!lobby)
        return;

    if (!ImGui::Begin("Lobby Room UI"))
    {
        ImGui::End();
        return;
    }

    ImGui::Checkbox("Force show ready/choose", &lobby->mDebugForceShowOverlays);
    ImGui::TextDisabled("(룸 입장 상태에서만 표시됩니다)");
    ImGui::Separator();

    static const char* names[] = { "Rudwig", "Ibanix", "Fanthor" };

    // 한 스프라이트(UITransform + 아틀라스 소스렉트)를 라이브 편집한다.
    auto editSprite = [this](const char* label, Entity e)
    {
        if (!e.IsValid())
            return;
        auto* tr = mWorld->GetComponent<UITransformComponent>(e);
        auto* sp = mWorld->GetComponent<UISpriteComponent>(e);
        if (!tr)
            return;

        ImGui::PushID(label);
        if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::DragFloat2("Pos",  &tr->mPosition.x, 1.0f);
            ImGui::DragFloat2("Size", &tr->mSize.x, 1.0f, 1.0f, 4096.0f);
            if (sp)
            {
                // SliderAngle: 라디안 포인터를 받아 도(deg)로 표시/편집
                ImGui::SliderAngle("Rotation", &sp->mRotation, -180.0f, 180.0f);
            }

            if (sp && sp->mUseSourceRect)
            {
                int rect[4] =
                {
                    static_cast<int>(sp->mSourceRect.left),
                    static_cast<int>(sp->mSourceRect.top),
                    static_cast<int>(sp->mSourceRect.right),
                    static_cast<int>(sp->mSourceRect.bottom),
                };
                if (ImGui::DragInt4("Src L,T,R,B", rect, 1.0f, 0, 2048))
                {
                    sp->mSourceRect.left   = rect[0];
                    sp->mSourceRect.top    = rect[1];
                    sp->mSourceRect.right  = rect[2];
                    sp->mSourceRect.bottom = rect[3];
                }
                ImGui::Text("Src WxH  %d x %d",
                    static_cast<int>(sp->mSourceRect.right - sp->mSourceRect.left),
                    static_cast<int>(sp->mSourceRect.bottom - sp->mSourceRect.top));
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    };

    // 현재 라이브 값을 kCharacterUiLayouts 형식 코드로 클립보드에 복사한다.
    if (ImGui::Button("Copy layout to clipboard"))
    {
        std::string out;
        char buf[512];
        for (uint8 i = 0; i < LobbyRoomSystem::DebugCharacterCount(); ++i)
        {
            Entity info, ready, choose[3], lock[3];
            std::array<Entity, 3> chooseArr{}, lockArr{};
            lobby->GetOverlayEntities(i, info, ready, chooseArr, lockArr);
            for (int k = 0; k < 3; ++k) { choose[k] = chooseArr[k]; lock[k] = lockArr[k]; }

            auto* infoTr = mWorld->GetComponent<UITransformComponent>(info);
            auto* infoSp = mWorld->GetComponent<UISpriteComponent>(info);
            auto* readyTr = mWorld->GetComponent<UITransformComponent>(ready);
            auto* readySp = mWorld->GetComponent<UISpriteComponent>(ready);
            auto* chooseSp = mWorld->GetComponent<UISpriteComponent>(choose[0]);
            auto* lockSp = mWorld->GetComponent<UISpriteComponent>(lock[0]);
            if (!infoTr || !infoSp || !readyTr || !readySp || !chooseSp)
                continue;

            const RECT& is = infoSp->mSourceRect;
            const RECT& rs = readySp->mSourceRect;
            const RECT& cs = chooseSp->mSourceRect;

            snprintf(buf, sizeof(buf),
                "// %s\n"
                "Info  src(%ld,%ld,%ld,%ld) pos(%.0f,%.0f) size(%.0f,%.0f)\n"
                "Ready src(%ld,%ld,%ld,%ld) pos(%.0f,%.0f) size(%.0f,%.0f)\n"
                "Choose src(%ld,%ld,%ld,%ld)\n",
                names[i],
                is.left, is.top, is.right - is.left, is.bottom - is.top,
                infoTr->mPosition.x, infoTr->mPosition.y, infoTr->mSize.x, infoTr->mSize.y,
                rs.left, rs.top, rs.right - rs.left, rs.bottom - rs.top,
                readyTr->mPosition.x, readyTr->mPosition.y, readyTr->mSize.x, readyTr->mSize.y,
                cs.left, cs.top, cs.right - cs.left, cs.bottom - cs.top);
            out += buf;

            if (lockSp)
            {
                const RECT& ls = lockSp->mSourceRect;
                snprintf(buf, sizeof(buf), "LockX src(%ld,%ld,%ld,%ld)\n",
                    ls.left, ls.top, ls.right - ls.left, ls.bottom - ls.top);
                out += buf;
            }

            for (int k = 0; k < 3; ++k)
            {
                auto* ctr = mWorld->GetComponent<UITransformComponent>(choose[k]);
                if (!ctr) continue;
                snprintf(buf, sizeof(buf),
                    "Choose[%d] offset(%.0f,%.0f) size(%.0f,%.0f)\n",
                    k, ctr->mPosition.x - infoTr->mPosition.x, ctr->mPosition.y - infoTr->mPosition.y,
                    ctr->mSize.x, ctr->mSize.y);
                out += buf;
            }
            out += "\n";
        }
        ImGui::SetClipboardText(out.c_str());
    }
    ImGui::Separator();

    for (uint8 i = 0; i < LobbyRoomSystem::DebugCharacterCount(); ++i)
    {
        Entity info, ready, choose[3], lock[3];
        std::array<Entity, 3> chooseArr{}, lockArr{};
        lobby->GetOverlayEntities(i, info, ready, chooseArr, lockArr);
        for (int k = 0; k < 3; ++k) { choose[k] = chooseArr[k]; lock[k] = lockArr[k]; }

        if (ImGui::CollapsingHeader(names[i], ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(i);
            editSprite("Info", info);
            editSprite("Ready", ready);
            editSprite("Choose 0", choose[0]);
            editSprite("Choose 1", choose[1]);
            editSprite("Choose 2", choose[2]);
            editSprite("Lock X 0", lock[0]);
            editSprite("Lock X 1", lock[1]);
            editSprite("Lock X 2", lock[2]);
            ImGui::PopID();
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawCameraInspector()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Camera Inspector"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<CameraComponent>())
    {
        ImGui::Text("CameraComponent 없음");
        ImGui::End();
        return;
    }

    auto cameras = mWorld->GetEntitiesWithComponent<CameraComponent>();
    int camIdx = 0;

    for (auto e : cameras)
    {
        CameraComponent* cam = mWorld->GetComponent<CameraComponent>(e);
        if (!cam) continue;

        char header[64];
        snprintf(header, sizeof(header), "Camera [%d] (Entity %u)", camIdx++, e);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 투영 타입
            int projType = static_cast<int>(cam->mCameraType);
            const char* projNames[] = { "Perspective", "Orthographic" };
            if (ImGui::Combo("Projection##Cam", &projType, projNames, 2))
                cam->mCameraType = static_cast<PROJECTION_TYPE>(projType);

            ImGui::Separator();
            ImGui::Text("Clip Plane");
            ImGui::DragFloat("Near##Cam",  &cam->mNear, 10.0f, 1.0f, 10000.0f);
            ImGui::DragFloat("Far##Cam",   &cam->mFar,  1000.0f, 1000.0f, 2000000.0f);

            ImGui::Separator();
            ImGui::Text("Shadow Clip");
            ImGui::DragFloat("ShadowNear##Cam", &cam->mShadowNear, 0.1f, 0.1f,  1000.0f);
            ImGui::DragFloat("ShadowFar##Cam",  &cam->mShadowFar,  100.0f, 100.0f, 100000.0f);

            ImGui::Separator();
            if (cam->mCameraType == PROJECTION_TYPE::PERSPECTIVE)
            {
                ImGui::SliderFloat("FOV (deg)##Cam", &cam->mFov, 10.0f, 90.0f);
            }
            else
            {
                ImGui::DragFloat("Scale##Cam", &cam->mScale, 0.01f, 0.01f, 100.0f);
            }

            // CameraTypeComponent (같은 엔티티에 붙어 있을 경우)
            CameraTypeComponent* camType = mWorld->GetComponent<CameraTypeComponent>(e);
            if (camType)
            {
                ImGui::Separator();
                ImGui::Text("CameraTypeComponent");

                const char* modeNames[] = { "MAIN_CAMERA", "1FPS", "3FPS", "3RPG" };
                ImGui::Text("PlayMode: %s", modeNames[static_cast<int>(camType->mPlayMode)]);

                ImGui::DragFloat("MoveSpeed##CamType",    &camType->mCameraMoveSpeed,   1.0f, 0.0f, 2000.0f);
                ImGui::DragFloat("MaxLength##CamType",    &camType->mCameraMaxLenth,    1.0f, 0.0f, 2000.0f);
                ImGui::DragFloat("MinLength##CamType",    &camType->mCameraMinLenth,    0.1f, 0.0f, 500.0f);
                ImGui::DragFloat("SphereRadius##CamType", &camType->mCameraSphereRadius, 0.5f, 0.0f, 200.0f);
                ImGui::DragFloat("Margin##CamType",       &camType->mCameraMargin,       0.1f, 0.0f, 100.0f);
                ImGui::DragFloat3("Offset##CamType",      &camType->mOffset.x,           1.0f);
                ImGui::DragFloat3("LookAtOffset##CamType",&camType->mLookAtOffset.x,     0.1f);
            }
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawLightInspector()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Light Inspector"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<LightComponent>())
    {
        ImGui::Text("LightComponent 없음");
        ImGui::End();
        return;
    }

    constexpr float kRad2Deg = 57.29577951f;
    constexpr float kDeg2Rad = 0.01745329252f;

    // UE 전방벡터 → DX 축 매핑(dx = ue.y, ue.z, ue.x) 을 적용한 결과.
    // dir = ( cosP*sinY, sinP, cosP*cosY )  — Scene.cpp 의 DirLightPrefab 값과 같은 규칙이다.
    auto dirFromPitchYaw = [](float pitchDeg, float yawDeg) -> Vec3
    {
        const float p = pitchDeg * kDeg2Rad;
        const float y = yawDeg * kDeg2Rad;
        const float cp = cosf(p);
        return Vec3(cp * sinf(y), sinf(p), cp * cosf(y));
    };

    auto lights = mWorld->GetEntitiesWithComponent<LightComponent>();
    int lightIdx = 0;

    for (auto e : lights)
    {
        LightComponent* light = mWorld->GetComponent<LightComponent>(e);
        if (!light) continue;

        const LIGHT_TYPE type = light->GetLightType();
        const char* typeNames[] = { "Directional", "Point", "Spot" };

        char header[80];
        snprintf(header, sizeof(header), "%s Light [%d] (Entity %u)",
            typeNames[static_cast<int>(type)], lightIdx++, e.GetID());

        if (!ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
            continue;

        ImGui::PushID(static_cast<int>(e.GetID()));

        if (type == LIGHT_TYPE::DIRECTIONAL_LIGHT)
        {
            const Vec4& d4 = light->mLightInfo.Direction;
            Vec3 dir(d4.x, d4.y, d4.z);

            // 슬라이더 상태를 따로 들지 않고 현재 방향에서 매 프레임 역산한다.
            // (다른 경로가 방향을 바꿔도 UI 가 항상 실제 값을 보여준다)
            float pitchDeg = asinf(max(-1.0f, min(1.0f, dir.y))) * kRad2Deg;
            float yawDeg = atan2f(dir.x, dir.z) * kRad2Deg;

            ImGui::Text("UE Rotation");
            bool angleChanged = false;
            angleChanged |= ImGui::SliderFloat("Pitch (deg)", &pitchDeg, -89.9f, 89.9f, "%.4f");
            // atan2 결과와 범위를 맞춘다. UE 의 Yaw=326.235 는 여기서 -33.765 로 보이며 같은 방향이다.
            angleChanged |= ImGui::SliderFloat("Yaw (deg)", &yawDeg, -180.0f, 180.0f, "%.4f");
            if (angleChanged)
                light->SetLightDirection(dirFromPitchYaw(pitchDeg, yawDeg));

            ImGui::Separator();
            ImGui::Text("Direction (DX, 정규화됨)");
            if (ImGui::DragFloat3("Direction", &dir.x, 0.005f, -1.0f, 1.0f, "%.4f"))
                light->SetLightDirection(dir);

            // 현재 방향/색상을 Scene.cpp 에 그대로 붙여넣을 수 있는 형태로 복사한다.
            if (ImGui::Button("Copy DirLightPrefab line"))
            {
                const Vec4& cur = light->mLightInfo.Direction;
                const LightColor& c = light->mLightInfo.Color;
                char buf[512];
                snprintf(buf, sizeof(buf),
                    "// UE DirLight (Pitch=%.6f, Yaw=%.6f)\n"
                    "DirLightPrefab light{ mWorld.get(), Vec3(%.4ff, %.4ff, %.4ff),\n"
                    "\t{ .diffuse  = { %.4ff, %.4ff, %.4ff },\n"
                    "\t  .ambient  = { %.4ff, %.4ff, %.4ff },\n"
                    "\t  .specular = { %.4ff, %.4ff, %.4ff } } };\n",
                    pitchDeg, yawDeg, cur.x, cur.y, cur.z,
                    c.Diffuse.x,  c.Diffuse.y,  c.Diffuse.z,
                    c.Ambient.x,  c.Ambient.y,  c.Ambient.z,
                    c.Specular.x, c.Specular.y, c.Specular.z);
                ImGui::SetClipboardText(buf);
            }

            ImGui::Separator();
            ImGui::DragFloat3("Position", &light->mLightInfo.Position.x, 1.0f);
            ImGui::TextDisabled("방향광의 Position 은 렌더에 사용되지 않는다");
        }
        else
        {
            ImGui::DragFloat3("Position", &light->mLightInfo.Position.x, 1.0f);
            ImGui::DragFloat("Range", &light->mLightInfo.Range, 1.0f, 0.0f, 100000.0f);
            ImGui::DragFloat("Angle", &light->mLightInfo.Angle, 0.1f, 0.0f, 180.0f);
        }

        ImGui::Separator();
        ImGui::Text("Color");
        ImGui::ColorEdit3("Diffuse", &light->mLightInfo.Color.Diffuse.x,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::ColorEdit3("Ambient", &light->mLightInfo.Color.Ambient.x,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
        ImGui::ColorEdit3("Specular", &light->mLightInfo.Color.Specular.x,
            ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

        ImGui::PopID();
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawWeaponTrailInspector()
{
#ifdef _IMGUI
    if (!ImGui::Begin("Weapon Trail Inspector"))
    {
        ImGui::End();
        return;
    }

    if (!mWorld->HasComponentPool<WeaponTrailComponent>())
    {
        ImGui::Text("WeaponTrailComponent 없음");
        ImGui::End();
        return;
    }

    
    static const char* styleNames[] = { "FlameRibbon", "SwordSlash", "HammerFlame" };
    static const WeaponTrailVisualStyle styleValues[] = {
        WeaponTrailVisualStyle::FlameRibbon,
        WeaponTrailVisualStyle::SwordSlash,
        WeaponTrailVisualStyle::HammerFlame,
    };

    auto entities = mWorld->GetEntitiesWithComponent<WeaponTrailComponent>();
    int trailIdx = 0;

    for (auto e : entities)
    {
        WeaponTrailComponent* trail = mWorld->GetComponent<WeaponTrailComponent>(e);
        if (!trail) continue;

        char header[64];
        snprintf(header, sizeof(header), "Trail [%d] (Entity %u)", trailIdx++, e);

        if (ImGui::CollapsingHeader(header, ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushID(static_cast<int>(e.GetID()));

            ImGui::Checkbox("Active", &trail->mIsActive);

            // 스타일 선택
            int styleSel = 0;
            for (int i = 0; i < IM_ARRAYSIZE(styleValues); ++i)
                if (styleValues[i] == trail->mVisualStyle) styleSel = i;
            if (ImGui::Combo("Style", &styleSel, styleNames, IM_ARRAYSIZE(styleNames)))
                trail->mVisualStyle = styleValues[styleSel];

            // Tip / Base 오프셋
            // tip/base = 트레일 리본의 폭 방향 양 끝. 칼날을 따라 눕도록.
            ImGui::Separator();
            ImGui::Text("Tip / Base Offset");
            if (trail->mSourceType == WeaponTrailSource::Socket)
            {
                // Socket 모드
                const Entity src = trail->mSourceEntity.IsValid() ? trail->mSourceEntity : e;
                SocketComponent* socketCom = mWorld->GetComponent<SocketComponent>(src);
                if (socketCom != nullptr)
                {
                    if (SocketDef* tipSocket = socketCom->FindSocket(trail->mTipSocketName))
                    {
                        Vec3 tipPos = tipSocket->mLocalOffset.Translation();
                        if (ImGui::DragFloat3("TipOffset", &tipPos.x, 0.5f))
                            tipSocket->mLocalOffset.Translation(tipPos);
                    }
                    else
                        ImGui::TextDisabled("Tip 소켓 '%s' 없음", trail->mTipSocketName.c_str());

                    if (SocketDef* baseSocket = socketCom->FindSocket(trail->mBaseSocketName))
                    {
                        Vec3 basePos = baseSocket->mLocalOffset.Translation();
                        if (ImGui::DragFloat3("BaseOffset", &basePos.x, 0.5f))
                            baseSocket->mLocalOffset.Translation(basePos);
                    }
                    else
                        ImGui::TextDisabled("Base 소켓 '%s' 없음", trail->mBaseSocketName.c_str());
                }
                else
                    ImGui::TextDisabled("SourceEntity에 SocketComponent 없음");
            }
            else
            {
                // Transform 모드
                ImGui::DragFloat3("TipOffset", &trail->mTipLocalOffset.x, 0.5f);
                ImGui::DragFloat3("BaseOffset", &trail->mBaseLocalOffset.x, 0.5f);
            }

            ImGui::Separator();
            ImGui::Text("Shape");
            int layerCount = static_cast<int>(trail->mLayerCount);
            if (ImGui::SliderInt("LayerCount", &layerCount, 1, 6))
                trail->mLayerCount = static_cast<uint32>(layerCount);
            int subdiv = static_cast<int>(trail->mSmoothingSubdivisions);
            if (ImGui::SliderInt("Smoothing", &subdiv, 0, 4))
                trail->mSmoothingSubdivisions = static_cast<uint32>(subdiv);
            ImGui::SliderFloat("WidthMultiplier", &trail->mWidthMultiplier, 0.0f, 4.0f);
            ImGui::SliderFloat("TailWidth", &trail->mTailWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("MidWidth", &trail->mMidWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("HeadWidth", &trail->mHeadWidthScale, 0.0f, 2.0f);
            ImGui::SliderFloat("LayerSpread", &trail->mLayerSpread, 0.0f, 1.0f);
            ImGui::SliderFloat("Lifetime", &trail->mLifetime, 0.02f, 0.6f);
            ImGui::SliderFloat("UvTiling", &trail->mUvTiling, 0.1f, 8.0f);

            ImGui::Separator();
            ImGui::Text("Slash Shading");
            ImGui::SliderFloat("BaseAlpha", &trail->mBaseAlpha, 0.0f, 1.0f);
            ImGui::SliderFloat("Intensity", &trail->mIntensity, 0.0f, 12.0f);
            ImGui::SliderFloat("CutStrength", &trail->mSlashCutStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("LineStrength", &trail->mSlashLineStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("EdgeBoost", &trail->mSlashEdgeBoost, 0.0f, 4.0f);
            ImGui::SliderFloat("CoreBoost", &trail->mSlashCoreBoost, 0.0f, 4.0f);
            ImGui::SliderFloat("TexScrollSpeed", &trail->mSlashTexScrollSpeed, 0.0f, 4.0f);

            ImGui::Separator();
            ImGui::Text("Colors");
            ImGui::ColorEdit3("CoreColor", &trail->mCoreColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::ColorEdit3("EdgeColor", &trail->mEdgeColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
            ImGui::ColorEdit3("SubColor",  &trail->mSubColor.x,  ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);

            ImGui::PopID();
        }
    }

    ImGui::End();
#endif
}

void IMGUIRenderSystem::DrawProperty(EditorProperty& prop)
{
#ifdef _IMGUI
        switch (prop.type)
        {
        case PropertyType::Bool:
            ImGui::Checkbox(prop.name.c_str(), (bool*)prop.data);
            break;

        case PropertyType::Float:
            ImGui::SliderFloat(
                prop.name.c_str(),
                (float*)prop.data,
                prop.min,
                prop.max
            );
            break;

        case PropertyType::Vec2:
            ImGui::DragFloat2(
                prop.name.c_str(),
                (float*)prop.data,
                1.0f
            );
            break;

        case PropertyType::Vec3:
            ImGui::DragFloat3(
                prop.name.c_str(),
                (float*)prop.data,
                0.1f
            );
            break;
        }
#endif
}

