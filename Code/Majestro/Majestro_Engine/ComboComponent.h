#pragma once
#include "Component.h"

class ComboComponent : public Component<ComboComponent>
{
public:
	int mCount = 0; // 현재 콤보 수 (서버가 보낸 값)
};
