#pragma once
class Game
{
public:

	void Initialize(const WindowInfo& info);
	void Update();
	void Input(UINT message);
	void Render();
};

