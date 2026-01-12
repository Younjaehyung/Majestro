#pragma once
class INetSendSink;

class Game
{
public:

	void Initialize(const WindowInfo& info);
	void Update();
	void Input(UINT message);
	void ActiveGame(bool active);
	void Render();

	int ImGuiInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

public: // network 
	void ReceiveNetworkData();
	void SendNetworkData();

public:
	bool mInitializeEnd = false;
	shared_ptr<INetSendSink> mSendSink;
};

