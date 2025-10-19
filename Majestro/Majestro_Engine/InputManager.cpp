#include "pch.h"
#include "InputManager.h"


//static멤버변수 이므로 전역에서 초기화함.

int ASCII[(UINT)eKeyCode::End] = {

	'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L',
'Z', 'X', 'C', 'V', 'B', 'N', 'M',VK_LEFT, VK_RIGHT, VK_DOWN, VK_UP,
VK_F1, VK_F2, VK_F3, VK_F4,
VK_ESCAPE,

};

inline HCURSOR CreateTransparentCursor() {
	ICONINFO ii{};
	ii.fIcon = FALSE; // 커서
	ii.xHotspot = 0;
	ii.yHotspot = 0;

	BITMAPV5HEADER bi{};
	bi.bV5Size = sizeof(bi);
	bi.bV5Width = 1;
	bi.bV5Height = -1;        // top-down
	bi.bV5Planes = 1;
	bi.bV5BitCount = 32;
	bi.bV5Compression = BI_BITFIELDS;
	bi.bV5RedMask = 0x00FF0000;
	bi.bV5GreenMask = 0x0000FF00;
	bi.bV5BlueMask = 0x000000FF;
	bi.bV5AlphaMask = 0xFF000000;

	void* bits = nullptr;
	HDC hdc = GetDC(nullptr);
	HBITMAP color = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &bits, nullptr, 0);
	ReleaseDC(nullptr, hdc);
	((DWORD*)bits)[0] = 0x00000000; // 완전 투명 픽셀

	HBITMAP mask = CreateBitmap(1, 1, 1, 1, nullptr); // 형식상 필요

	ii.hbmColor = color;
	ii.hbmMask = mask;

	HCURSOR cur = (HCURSOR)CreateIconIndirect(&ii);

	// CreateIconIndirect가 자체 복사본을 가지므로 원본 비트맵은 해제 가능
	DeleteObject(color);
	DeleteObject(mask);
	return cur;
}

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

void InputManager::MouseStateClear() {
	mMouseState.Delta = { 0,0 };
}

void InputManager::OnMouseMove(LPARAM lParam)
{
	
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
		mMouseState.LeftDown = true;

		mMouseState.Delta = { 0,0 };

		::GetCursorPos(&mMouseState.Position);
		::GetCursorPos(&mMouseState.OldPosition);
		::GetCursorPos(&mMouseState.ClickPosition);
		
		break;

	case WM_RBUTTONDOWN:
		mMouseState.RightDown = true;
		break;

	case WM_LBUTTONUP:
		mMouseState.LeftDown = false;
		::ReleaseCapture();

		::SetCursor(arrow);
		::SetCursorPos(mMouseState.ClickPosition.x, mMouseState.ClickPosition.y);
		break;

	case WM_RBUTTONUP:
		mMouseState.RightDown = false;
		::ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		if (mMouseState.LeftDown) { 
			::GetCursorPos(&mMouseState.Position);

			mMouseState.Delta.x += mMouseState.Position.x - mMouseState.OldPosition.x;
			mMouseState.Delta.y += mMouseState.Position.y - mMouseState.OldPosition.y;

			::SetCursorPos(mMouseState.ClickPosition.x, mMouseState.ClickPosition.y); 
			mMouseState.OldPosition = mMouseState.ClickPosition;
			mMouseState.Position = mMouseState.ClickPosition;
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