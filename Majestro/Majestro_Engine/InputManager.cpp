#include "pch.h"
#include "InputManager.h"


//static멤버변수 이므로 전역에서 초기화함.

int ASCII[(UINT)eKeyCode::End] = {

	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
'Z', 'X', 'C', 'V', 'B', 'N', 'M',VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP,VK_F1, VK_ESCAPE,

};

void InputManager::Initialize(HWND _hWnd) {

	for (size_t i = 0; i < (UINT)eKeyCode::End; i++) {

		Key key = {};
		key.bPressed = false;
		key.state = eKeyState::None;
		key.keyCode = (eKeyCode)i;

		mKeys.push_back(key);
	}
	mHwnd = _hWnd;
}

void InputManager::Update() {
	for (size_t i = 0; i < mKeys.size(); i++) {
		//눌렀는지
		if (GetAsyncKeyState(ASCII[i]) & 0x8000) {
			if (mKeys[i].bPressed == true) {
				mKeys[i].state = eKeyState::Pressed;
			}
			else {
				mKeys[i].state = eKeyState::Down;
			}
			mKeys[i].bPressed = true;
		}
		else {// 아닌지

			if (mKeys[i].bPressed == true) {
				mKeys[i].state = eKeyState::Up;
			}
			else {
				mKeys[i].state = eKeyState::None;
			}
			mKeys[i].bPressed = false;
		}


	}
}


void InputManager::OnMouseMove(LPARAM lParam)
{
	int x = LOWORD(lParam);
	int y = HIWORD(lParam);
	mMouseState.Delta.x = x - mMouseState.Position.x;
	mMouseState.Delta.y = y - mMouseState.Position.y;

	mMouseState.Position.x = x;
	mMouseState.Position.y = y;

}

void InputManager::OnMouseWheel(WPARAM wParam)
{
	mMouseState.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
}

void InputManager::OnMouseButtonDown(WPARAM button)
{
	::SetCapture(mHwnd);
	if (button == MK_LBUTTON) mMouseState.LeftDown = true;
	if (button == MK_RBUTTON) mMouseState.RightDown = true;
	if (button == MK_MBUTTON) mMouseState.MiddleDown = true;
}

void InputManager::OnMouseButtonUp(WPARAM button)
{

	::ReleaseCapture();
	switch (button)
	{
	case WM_LBUTTONUP:
		mMouseState.LeftDown = false;
		break;
	case WM_RBUTTONUP:
		mMouseState.RightDown = false;
		break;
	case WM_MBUTTONUP:
		mMouseState.MiddleDown = false;
		break;
	default:

		if ((button & MK_LBUTTON) == 0) mMouseState.LeftDown = false;
		if ((button & MK_RBUTTON) == 0) mMouseState.RightDown = false;
		if ((button & MK_MBUTTON) == 0) mMouseState.MiddleDown = false;
		break;
	}
}