// ----------------------------------------------------
// ウィンドウ設定 [game_window.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-11
// Version: 1.0
// ----------------------------------------------------

#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

HWND GameWindow_Create(_In_ HINSTANCE hInstance);

void GameWindow_ToggleFullScreen();

#endif GAME_WINDOW_H
