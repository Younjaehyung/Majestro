#pragma once
class Game
{
public:

	void Initialize(const WindowInfo& info);
	void Update();
	void Input(UINT message);
	void Render();

	int ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

