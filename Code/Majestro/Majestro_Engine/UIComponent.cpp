#include "pch.h"
#include "UIComponent.h"
#include "Entity.h"
#include "PlayerComponent.h"

PlayerHpTextureNames GetPlayerHpTextureNames(uint8 playerType)
{
	switch (static_cast<PlayerType>(playerType))
	{
	case PlayerType::Rudwig: return { L"UI_Rudwig_HP_0", L"UI_Rudwig_HP_1" };
	case PlayerType::Ibanix: return { L"UI_Ibanix_HP_0", L"UI_Ibanix_HP_1" };
	default:                 return { L"UI_Fanthor_HP_0", L"UI_Fanthor_HP_1" };
	}
}


