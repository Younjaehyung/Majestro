#pragma once
#include <vector>

//숫자를 문자로 변환해주는 클래스임
enum class eKeyState {
	Down,//0
	Pressed,//1
	Up,//2
	None,//3
};

enum class eKeyCode {
	Q, W, E, R, T, Y, U, I, O, P,
	A, S, D, F, G, H, J, K, L,
	Z, X, C, V, B, N, M, LEFT, RIGHT, DOWN, UP, SPACE, SHIFT,
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9,
	F1, F2, F3, F4,
	ESC, GRAVE, End,
};

struct MouseState
{
	POINT Position = { 0, 0 };
	POINT OldPosition = { 0, 0 };
	POINT ClickPosition = { 500, 500 }; //클릭한 위치
	POINT Delta = { 0, 0 }; // 이전 프레임과의 차이
	short WheelDelta = 0;

	bool LeftDown = false;
	bool RightDown = false;
	bool MiddleDown = false;
};

enum class eMouseInputMode
{
	LegacyRelative,
	RecenterRelative,
};

class InputManager
{
public:
	struct Key {
		eKeyCode keyCode;
		eKeyState state;
		bool bPressed;
	};
	void Initialize(HWND _hWnd);
	void Update();

	bool GetKeyDown(eKeyCode code) {
		return mKeys[(UINT)code].state == eKeyState::Down;
	}
	bool GetKey(eKeyCode code) {
		return mKeys[(UINT)code].state == eKeyState::Pressed;
	}
	bool GetKeyUp(eKeyCode code) {
		return mKeys[(UINT)code].state == eKeyState::Up;
	}

	void OnActivateApp(bool active);
	void OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam);
	void OnMouseWheel(WPARAM wParam);
	void OnMouseButtonDown(WPARAM button);
	void OnMouseButtonUp(WPARAM button);
	void SetForceMouseLook(bool enable);
	bool IsMouseLookActive() const { return mMouseLookControl; }
	void SetMouseInputMode(eMouseInputMode mode);
	eMouseInputMode GetMouseInputMode() const { return mMouseInputMode; }

	const MouseState& GetMouseState() const { return mMouseState; }
	void MouseStateClear();


private:
	void HideCursor();
	void ShowCursorRestore();

	HWND mHwnd;
	bool mHasFocus = true;                         // 창이 활성화 상태인지
	bool mMouseLookControl = false;
	bool mForceMouseLookRequested = false;
	bool mCursorHidden = false;                    // ShowCursor 상태 추적
	eMouseInputMode mMouseInputMode = eMouseInputMode::LegacyRelative;
	MouseState mMouseState;

	std::vector<Key> mKeys;
};

