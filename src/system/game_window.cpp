// --------------------------------------------
// ウィンドウ設定 [game_window.cpp]
// ============================================
// Created by: Yasuda Atsushi
// Date: 2025-06-18
// Version: 1.0
//---------------------------------------------
#include "game_window.h"
#include <algorithm>
#include "keyboard.h"
#include "mouse.h"
#include "system_timer.h"

// ウィンドウ情報
static constexpr char WINDOW_CLASS[] = "GameWindow"; // メインウインドウクラス名
static constexpr char TITLE[] = "Game";	// 	タイトルバーのテクスト
const constexpr int SCREEN_WIDTH = 1920;
const constexpr int SCREEN_HEIGHT = 1080;

static bool g_isFullScreen = false;
static HWND g_hWnd = nullptr; // 初期化

// ウインドウプロシージャ　プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND GameWindow_Create(_In_ HINSTANCE hInstance)
{
	// ウインドウクラスの登録
	WNDCLASSEX wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = WINDOW_CLASS;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	RegisterClassEx(&wcex);

	// メインウィンドウの作成

	RECT window_rect{
		0, 0, SCREEN_WIDTH, SCREEN_HEIGHT
	};

	DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME);

	AdjustWindowRect(&window_rect, style, FALSE);

	const int WINDOW_WIDTH = window_rect.right - window_rect.left;
	const int WINDOW_HEIGHT = window_rect.bottom - window_rect.top;

	// デスクトップのサイズを取得
	// ライブラリモニターの画面解像度取得
	int desktop_width = GetSystemMetrics(SM_CXSCREEN);
	int desktop_height = GetSystemMetrics(SM_CYSCREEN);

	// ウインドウの表示位置をデスクトップの真ん中に調整する
	// offset to center, use std::max to prevent window overflow
	const int WINDOW_X = std::max(0, (desktop_width - WINDOW_WIDTH) / 2);
	const int WINDOW_Y = std::max(0, (desktop_height - WINDOW_HEIGHT) / 2);

	HWND hWnd = CreateWindow(
		WINDOW_CLASS,
		TITLE,
		style,	//　Window Style Flag
		WINDOW_X,	// CW_USEDEFAULT
		WINDOW_Y,
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);

	g_hWnd = hWnd; // ここで保存

	// ここで起動時からフルスクリーンにする
	if (g_hWnd) {
		SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(
			g_hWnd,
			HWND_TOP,
			0, 0,
			GetSystemMetrics(SM_CXSCREEN),
			GetSystemMetrics(SM_CYSCREEN),
			SWP_SHOWWINDOW
		);
		g_isFullScreen = true;
	}

	return hWnd;
}

// ウインドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_ACTIVATE:
		if (wParam != WA_INACTIVE) {
			SystemTimer_Start();
		}
		break;
	case WM_ACTIVATEAPP:
		Keyboard_ProcessMessage(message, wParam, lParam);
		Mouse_ProcessMessage(message, wParam, lParam);
		SystemTimer_Stop();
		break;
	case WM_INPUT:
		Mouse_ProcessMessage(message, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEWHEEL:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEHOVER:
	    Mouse_ProcessMessage(message, wParam, lParam);
	    break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE){
			SendMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		}
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	    Keyboard_ProcessMessage(message, wParam, lParam);
	    break;

	case WM_CLOSE:
		if (MessageBoxW(hWnd, L"本当に終了してよろしいですか？", L"確認", MB_YESNO | MB_DEFBUTTON2) == IDYES)
		{
			DestroyWindow(hWnd);
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

void GameWindow_ToggleFullScreen()
{
	if (!g_hWnd) return;

	if (!g_isFullScreen) {
		// フルスクリーン化
		SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(g_hWnd, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
		g_isFullScreen = true;
	}
	else {
		// ウィンドウモードに戻す
		SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		// 画面中央に戻す
		int desktop_width = GetSystemMetrics(SM_CXSCREEN);
		int desktop_height = GetSystemMetrics(SM_CYSCREEN);
		int window_width = SCREEN_WIDTH;
		int window_height = SCREEN_HEIGHT;
		int window_x = std::max(0, (desktop_width - window_width) / 2);
		int window_y = std::max(0, (desktop_height - window_height) / 2);

		SetWindowPos(g_hWnd, HWND_TOP, window_x, window_y, window_width, window_height, SWP_SHOWWINDOW);
		g_isFullScreen = false;
	}
}
