#pragma once
class Game
{
public:

	void Initialize(const WindowInfo& info);
	void Update();
	void Input(UINT message);
	void ActiveGame(bool active);
	void Render();

	int ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public:
	bool mInitializeEnd = false;
};

