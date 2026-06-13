#pragma once
#include "Component.h"

class BeatComponent : public Component<BeatComponent>
{
public:
	int mBeat;
};