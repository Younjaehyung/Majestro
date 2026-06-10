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
	F1, F2, F3, F4, F5,
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

enum class eMouseButton
{
	Left = 0,
	Right,
	Middle,
	End
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
	bool GetAnyKeyDown();


	bool GetMouseDown(eMouseButton button) const {
		return mMouseDownEvent[static_cast<size_t>(button)];
	}
	bool GetMouse(eMouseButton button) const {
		return mMousePressed[static_cast<size_t>(button)];
	}
	bool GetMouseUp(eMouseButton button) const {
		return mMouseUpEvent[static_cast<size_t>(button)];
	}
	bool GetMouseLeftDown() const { return GetMouseDown(eMouseButton::Left); }
	bool GetMouseLeft() const { return GetMouse(eMouseButton::Left); }
	bool GetMouseLeftUp() const { return GetMouseUp(eMouseButton::Left); }

	bool GetMouseRightDown() const {
		return GetMouseDown(eMouseButton::Right);
	}
	bool GetMouseRight() const {
		return GetMouse(eMouseButton::Right);
	}
	bool GetMouseRightUp() const {
		return GetMouseUp(eMouseButton::Right);
	}
	bool GetMouseMiddleDown() const { return GetMouseDown(eMouseButton::Middle); }
	bool GetMouseMiddle() const { return GetMouse(eMouseButton::Middle); }
	bool GetMouseMiddleUp() const { return GetMouseUp(eMouseButton::Middle); }

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
	bool GetClientClipRect(RECT& outRect) const;

	void HideCursor();
	void ShowCursorRestore();

	void LockCursorToClient();
	void UnlockCursor();

	void CenterCursorInClient();
	void RefreshCursorLock();

	HWND mHwnd = nullptr;
	bool mHasFocus = true;                         // 창이 활성화 상태인지
	bool mMouseLookControl = false;
	bool mForceMouseLookRequested = false;
	bool mCursorHidden = false;                    // ShowCursor 상태 추적
	bool mCursorLocked = false;                    // Cursor Lock 상태

	eMouseInputMode mMouseInputMode = eMouseInputMode::RecenterRelative;
	MouseState mMouseState;

	bool mPrevMousePressed[static_cast<size_t>(eMouseButton::End)] = { false, false, false };
	bool mMousePressed[static_cast<size_t>(eMouseButton::End)] = { false, false, false };
	bool mMouseDownEvent[static_cast<size_t>(eMouseButton::End)] = { false, false, false };
	bool mMouseUpEvent[static_cast<size_t>(eMouseButton::End)] = { false, false, false };

	std::vector<Key> mKeys;
};

