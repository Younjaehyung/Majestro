#include "EnginePch.h"
#include "ChatOverlay.h"
#include "Chat.h"

// 채팅 오버레이 (로그 + 입력줄).
void DrawChatOverlay(const UITextOverlayContext& ctx)
{
    Chat& chat = Chat::Get();
    const bool focused = chat.IsFocused();
    const auto& log = chat.GetLog();
    if (!focused && log.empty())
        return;


    std::shared_ptr<DirectX::SpriteFont> font = ctx.getFont(UIFontType::Esamanru);
    if (font == nullptr)
        font = ctx.getFont(UIFontType::Arial);
    if (font == nullptr)
        return;

    const Vec2 screenSize = ctx.screenSize;

 
    const float s = std::min(screenSize.x / 2560.f, screenSize.y / 1440.f);
    const float textScale = 1.05f * s;
    const float lineHeight = (font->GetLineSpacing() + 8.f) * textScale;
    const float baseX = 60.f * s;
    const float inputY = screenSize.y - 150.f * s;


    static const wchar_t* kSenderNames[] = { L"Rudwig", L"Ibanix", L"Fanthor" };
    static const DirectX::XMVECTORF32 kSenderColors[] = {
        { { { 0.55f, 0.95f, 0.55f, 1.f } } },   // Rudwig
        { { { 0.55f, 0.80f, 1.00f, 1.f } } },   // Ibanix
        { { { 1.00f, 0.72f, 0.42f, 1.f } } },   // Fanthor
    };
    static const DirectX::XMVECTORF32 kWhite = { { { 1.f, 1.f, 1.f, 1.f } } };
    static const DirectX::XMVECTORF32 kAccent = { { { 1.f, 0.95f, 0.55f, 1.f } } };

    // 그림자 + 본문을 그리고, 그린 문자열의 픽셀 폭을 반환
    auto drawText = [&](const std::wstring& text, float x, float y,
        DirectX::XMVECTORF32 color, float alpha) -> float
        {
            if (text.empty())
                return 0.f;

            color.f[3] *= alpha;
            DirectX::XMVECTORF32 shadow = { { { 0.f, 0.f, 0.f, 0.8f } } };
            shadow.f[3] *= alpha;

            const DirectX::SimpleMath::Vector2 pos(x, y);
            const DirectX::SimpleMath::Vector2 shadowPos(x + 2.f * s, y + 2.f * s);
            const XMFLOAT2 origin{ 0.f, 0.f };

            font->DrawString(ctx.batch, text.c_str(), shadowPos, shadow, 0.f, origin, textScale);
            font->DrawString(ctx.batch, text.c_str(), pos, color, 0.f, origin, textScale);

            XMFLOAT2 size;
            XMStoreFloat2(&size, font->MeasureString(text.c_str()));
            return size.x * textScale;
        };

    // 로그
    const int maxVisible = focused ? 12 : 8;
    int drawn = 0;
    for (auto it = log.rbegin(); it != log.rend() && drawn < maxVisible; ++it)
    {
        const float alpha = focused ? 1.f : chat.GetMessageAlpha(*it);
        if (alpha <= 0.f)
            break;  // 뒤로 갈수록 더 오래된 메시지라 전부 투명

        const float y = inputY - lineHeight * static_cast<float>(drawn + 1);

        const bool knownSender = it->playerType < _countof(kSenderNames);
        const std::wstring prefix = knownSender
            ? (L"[" + std::wstring(kSenderNames[it->playerType]) + L"] ")
            : L"[???] ";
        const DirectX::XMVECTORF32 prefixColor = knownSender
            ? kSenderColors[it->playerType] : kWhite;

        float x = baseX;
        x += drawText(prefix, x, y, prefixColor, alpha);
        drawText(it->text, x, y, kWhite, alpha);
        ++drawn;
    }

    // 입력줄: "채팅: <확정 입력><조합 중 글자><캐럿>"
    if (focused)
    {
        float x = baseX;
        x += drawText(L"채팅: ", x, inputY, kAccent, 1.f);
        x += drawText(chat.GetInput(), x, inputY, kWhite, 1.f);
        x += drawText(chat.GetComposition(), x, inputY, kAccent, 1.f);
        if (chat.IsCaretVisible())
            drawText(L"|", x, inputY, kWhite, 1.f);
    }
}
