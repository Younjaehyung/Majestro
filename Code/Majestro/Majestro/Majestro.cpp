// Majestro.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "Majestro.h"
#include "Game.h"
#include "Scene.h"
#include "SceneManager.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.
WindowInfo gWindowInfo;
bool gIsInitializing = true;
ULONG_PTR gGdiplusToken = 0;

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void DrawStartupLoadingFrame(HWND hWnd);
bool InitializeGdiPlus();
void ShutdownGdiPlus();

unique_ptr<Game> game = make_unique<Game>();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    InitializeGdiPlus();

    // TODO: 여기에 코드를 입력합니다.
    // 
#ifdef _IMGUI
    // WinMain or Engine 시작부
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
#else
#endif


    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAJESTRO, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        ShutdownGdiPlus();
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAJESTRO));

    MSG msg;


    RECT clientRect{};
    ::GetClientRect(gWindowInfo.Hwnd, &clientRect);
    gWindowInfo.Width = clientRect.right - clientRect.left;
    gWindowInfo.Height = clientRect.bottom - clientRect.top;
    gWindowInfo.ScreenState = true;

    DrawStartupLoadingFrame(gWindowInfo.Hwnd);
    RedrawWindow(gWindowInfo.Hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);

    game->Initialize(gWindowInfo);
    gIsInitializing = false;

    // 기본 메시지 루프입니다:
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                break;

            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            // DestroyWindow 등으로 WM_QUIT가 큐에 쌓인 경우 즉시 탈출
            MSG peekQuit{};
            if (PeekMessage(&peekQuit, nullptr, WM_QUIT, WM_QUIT, PM_NOREMOVE))
            {
                msg = peekQuit;
                break;
            }
        }

        game->Update();
        game->Render();
    }

    ShutdownGdiPlus();
    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAJESTRO));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    /*wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);*/
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName   = nullptr;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{

   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 2560, 1440, nullptr, nullptr, hInstance, nullptr);
   //mWindow.Width = 2560;
   //mWindow.Height = 1440;
   gWindowInfo.Hwnd = hWnd;


   if (!hWnd)
   {
      return FALSE;
   }


   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

bool InitializeGdiPlus()
{
    if (gGdiplusToken != 0)
        return true;

    Gdiplus::GdiplusStartupInput startupInput;
    return Gdiplus::GdiplusStartup(&gGdiplusToken, &startupInput, nullptr) == Gdiplus::Ok;
}

void ShutdownGdiPlus()
{
    if (gGdiplusToken == 0)
        return;

    Gdiplus::GdiplusShutdown(gGdiplusToken);
    gGdiplusToken = 0;
}

void DrawStartupLoadingFrame(HWND hWnd)
{
    if (!hWnd)
        return;

    RECT rect{};
    GetClientRect(hWnd, &rect);

    HDC hdc = GetDC(hWnd);
    if (!hdc)
        return;

    bool imageDrawn = false;
    if (gGdiplusToken != 0)
    {
        Gdiplus::Graphics graphics(hdc);
        Gdiplus::Image image(L"..\\Resources\\Image\\UI\\UI_Main_Title.png");
        if (image.GetLastStatus() == Gdiplus::Ok)
        {
            const Gdiplus::Rect destRect(
                static_cast<INT>(rect.left),
                static_cast<INT>(rect.top),
                static_cast<INT>(rect.right - rect.left),
                static_cast<INT>(rect.bottom - rect.top));
            graphics.DrawImage(&image, destRect);
            imageDrawn = true;
        }
    }

    if (!imageDrawn)
    {
        HBRUSH background = CreateSolidBrush(RGB(8, 8, 12));
        FillRect(hdc, &rect, background);
        DeleteObject(background);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(230, 230, 230));
        DrawTextW(hdc, L"LOADING...", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    ReleaseDC(hWnd, hdc);
}



inline HCURSOR CreateTransparentCursorMask32() //투명커서
{
    static HCURSOR transparentCursor = nullptr;
    if (transparentCursor)
        return transparentCursor;

    BYTE ANDmask[32 * 4] = {}; // 모두 0xFF로 채워도 됨(장치 종속 비트맵 규칙)
    memset(ANDmask, 0xFF, sizeof(ANDmask));
    BYTE XORmask[32 * 4] = {}; // 0으로 유지 → 투명
    transparentCursor = CreateCursor(GetModuleHandle(nullptr), 0, 0, 32, 32, ANDmask, XORmask);
    return transparentCursor;
}


//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
#ifdef _IMGUI
	if (game->ImGuiInput(hWnd, message, wParam, lParam))
        return true;
#else
#endif
    const bool isGameScene = game->IsGameSceneActive();

    if (isGameScene)
    {
        ::SetCursor(CreateTransparentCursorMask32());
    }

    //if(game)game->Input(message);
    switch (message)
    {
    case WM_ERASEBKGND:
        if (gIsInitializing)
            return TRUE;
        break;

    case WM_SETCURSOR:
        if (isGameScene)
            return TRUE;
        break;
    case WM_ACTIVATEAPP:
        game->ActiveGame(wParam != 0); // [추가]
        return 0;

    case WM_SETFOCUS:
        game->ActiveGame(true);        // [추가]
        return 0;

    case WM_KILLFOCUS:
        game->ActiveGame(false);       // [추가]
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hWnd);
            return 0;
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
        /*::SetCursor(CreateTransparentCursorMask32());
        SetCapture(hWnd);
        [[fallthrough]];*/
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MOUSEMOVE:
        game->Input(message, wParam, lParam);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 메뉴 선택을 구문 분석합니다:
            switch (wmId)
            {
                

            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
  
            BeginPaint(hWnd, &ps);

            if (gIsInitializing)
            {
                EndPaint(hWnd, &ps);
                DrawStartupLoadingFrame(hWnd);
                return 0;
            }

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
