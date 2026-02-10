#pragma once

class Game
{
public:

	void Initialize(const WindowInfo& info);
	void Update();
	void Input(UINT message);
	void ActiveGame(bool active);
	void Render();
	bool IsGameSceneActive() const;

	int ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public: // network 
	void ReceiveNetworkData();
	void SendNetworkData();

public:
	bool mInitializeEnd = false;

};

