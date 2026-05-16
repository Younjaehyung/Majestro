#pragma once
#include "Component.h"

class BeatComponent : public Component<BeatComponent>
{
public:
	int	 mBeat  = 168;
	bool mBouns = false;
};