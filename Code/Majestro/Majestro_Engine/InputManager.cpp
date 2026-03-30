#include "pch.h"
#include "InputManager.h"


//static멤버변수 이므로 전역에서 초기화함.

int ASCII[(UINT)eKeyCode::End] = {

	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
'Z', 'X', 'C', 'V', 'B', 'N', 'M',VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP, VK_SPACE, VK_SHIFT,
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
VK_F1, VK_F2, VK_F3, VK_F4,
VK_ESCAPE,
VK_OEM_3,   // ` 백틱

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
	if (!mHasFocus)
		return;

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

	mMousePressed[static_cast<size_t>(eMouseButton::Left)] = mMouseState.LeftDown;
	mMousePressed[static_cast<size_t>(eMouseButton::Right)] = mMouseState.RightDown;
	mMousePressed[static_cast<size_t>(eMouseButton::Middle)] = mMouseState.MiddleDown;

	for (size_t i = 0; i < static_cast<size_t>(eMouseButton::End); ++i)
	{
		mMouseDownEvent[i] = (!mPrevMousePressed[i] && mMousePressed[i]);
		mMouseUpEvent[i] = (mPrevMousePressed[i] && !mMousePressed[i]);
		mPrevMousePressed[i] = mMousePressed[i];
	}
}

void InputManager::HideCursor()
{
	if (!mCursorHidden)
	{
		ShowCursor(FALSE);
		mCursorHidden = true;
	}
}

void InputManager::ShowCursorRestore()
{
	if (mCursorHidden)
	{
		ShowCursor(TRUE);
		mCursorHidden = false;
	}
}

void InputManager::SetForceMouseLook(bool enable)
{
	mForceMouseLookRequested = enable;
	if (mMouseLookControl == enable)
		return;

	mMouseLookControl = enable;
	mMouseState.Delta = { 0, 0 };

	if (mMouseLookControl)
	{
		// 현재 커서 위치를 고정점으로 저장
		::GetCursorPos(&mMouseState.Position);
		//::GetCursorPos(&mMouseState.OldPosition);
		::GetCursorPos(&mMouseState.ClickPosition);

		if (mMouseInputMode == eMouseInputMode::LegacyRelative)
		{
			POINT clientPos = mMouseState.Position;
			::ScreenToClient(mHwnd, &clientPos);
			mMouseState.Position = clientPos;
			mMouseState.OldPosition = clientPos;
		}
		else
		{
			mMouseState.OldPosition = mMouseState.Position;
		}

		HideCursor();
	}
	else
	{
		ShowCursorRestore();
	}
}

void InputManager::OnActivateApp(bool active)
{
	mHasFocus = active;

	if (!mHasFocus)
	{
		// 포커스 잃을 때: 커서 복원 (Alt+Tab 등으로 Up 메시지가 안 올 수 있음)
		ShowCursorRestore();
		mMouseLookControl = false;

		// 마우스 버튼 상태 초기화
		mMouseState.LeftDown = false;
		mMouseState.RightDown = false;
		mMouseState.MiddleDown = false;
		mMouseState.Delta = { 0, 0 };
		mMouseState.WheelDelta = 0;
		for (size_t i = 0; i < static_cast<size_t>(eMouseButton::End); ++i)
		{
			mPrevMousePressed[i] = false;
			mMousePressed[i] = false;
			mMouseDownEvent[i] = false;
			mMouseUpEvent[i] = false;
		}
		// 키 상태 초기화 (Sticky 입력 방지)
		for (auto& k : mKeys)
		{
			k.bPressed = false;
			k.state = eKeyState::None;
		}
		return;
	}

	// 포커스 복귀 시 마우스 룩 재설정
	if (mForceMouseLookRequested)
		SetForceMouseLook(true);
}

void InputManager::SetMouseInputMode(eMouseInputMode mode)
{
	if (mMouseInputMode == mode)
		return;

	mMouseInputMode = mode;
	mMouseState.Delta = { 0, 0 };

	if (!mMouseLookControl)
		return;

	::GetCursorPos(&mMouseState.Position);
	::GetCursorPos(&mMouseState.ClickPosition);

	if (mMouseInputMode == eMouseInputMode::LegacyRelative)
	{
		POINT clientPos = mMouseState.Position;
		::ScreenToClient(mHwnd, &clientPos);
		mMouseState.Position = clientPos;
		mMouseState.OldPosition = clientPos;
	}
	else
	{
		mMouseState.OldPosition = mMouseState.Position;
	}
}

void InputManager::MouseStateClear() {
	mMouseState.Delta = { 0, 0 };
}

void InputManager::OnMouseEvent(UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_LBUTTONDOWN:
		mMouseState.LeftDown = true;
		break;

	case WM_RBUTTONDOWN:
		mMouseState.RightDown = true;
		break;
	case WM_MBUTTONDOWN:
		mMouseState.MiddleDown = true;
		break;

	case WM_LBUTTONUP:
		mMouseState.LeftDown = false;
		break;

	case WM_RBUTTONUP:
		mMouseState.RightDown = false;
		break;
	case WM_MBUTTONUP:
		mMouseState.MiddleDown = false;
		break;

	case WM_MOUSEMOVE:
		if (mMouseLookControl) {
			if (mMouseInputMode == eMouseInputMode::LegacyRelative)
			{
				POINT clientPos = {
					static_cast<LONG>(static_cast<SHORT>(LOWORD(lParam))),
					static_cast<LONG>(static_cast<SHORT>(HIWORD(lParam)))
				};
				mMouseState.Position = clientPos;
				mMouseState.Delta.x += mMouseState.Position.x - mMouseState.OldPosition.x;
				mMouseState.Delta.y += mMouseState.Position.y - mMouseState.OldPosition.y;
				mMouseState.OldPosition = mMouseState.Position;
			}
			else
			{
				::GetCursorPos(&mMouseState.Position);

				mMouseState.Delta.x += mMouseState.Position.x - mMouseState.OldPosition.x;
				mMouseState.Delta.y += mMouseState.Position.y - mMouseState.OldPosition.y;

				// 커서를 고정 위치로 복귀 (다음 WM_MOUSEMOVE에서 Delta=0이 됨)
				::SetCursorPos(mMouseState.ClickPosition.x, mMouseState.ClickPosition.y);
				mMouseState.OldPosition = mMouseState.ClickPosition;
				mMouseState.Position = mMouseState.ClickPosition;
			}
		}
		else
		{
			// 마우스 룩 모드 아닐 때 (UI 모드): lParam의 클라이언트 좌표로 갱신
			// UIButtonSystem HitTest가 이 값을 사용하므로 반드시 갱신 필요
			mMouseState.Position = {
				static_cast<LONG>(static_cast<SHORT>(LOWORD(lParam))),
				static_cast<LONG>(static_cast<SHORT>(HIWORD(lParam)))
			};
		}
		break;
	default:
		break;
	}

}

void InputManager::OnMouseWheel(WPARAM wParam)
{
	mMouseState.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
}

void InputManager::OnMouseButtonDown(WPARAM button)
{
	if (button == MK_LBUTTON) mMouseState.LeftDown = true;
	if (button == MK_RBUTTON) mMouseState.RightDown = true;
	if (button == MK_MBUTTON) mMouseState.MiddleDown = true;
}

void InputManager::OnMouseButtonUp(WPARAM button)
{
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
