// ----------------------------------------------------
// パッド入力処理 [pad_logger.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-03
// Version: 1.0
// ----------------------------------------------------
#ifndef PAD_LOGGER_H
#define PAD_LOGGER_H

#include <Windows.h>
#include <Xinput.h>
#include <DirectXMath.h>

void PadLogger_Initialize();

void PadLogger_Update();

// ここはボタン系だけ
bool PadLogger_IsPressed(DWORD user_index, WORD buttons);
bool PadLogger_IsTrigger(DWORD user_index, WORD buttons);
bool PadLogger_IsRelease(DWORD user_index, WORD buttons);

// ここでレバーを取る
DirectX::XMFLOAT2 PadLogger_GetLeftThumbStick(DWORD user_index);
DirectX::XMFLOAT2 PadLogger_GetRightThumbStick(DWORD user_index);
float PadLogger_GetLeftTrigger(DWORD user_index);
float PadLogger_GetRightTrigger(DWORD user_index);

void PadLogger_VibrationEnable(DWORD user_index, bool enable);

#endif // PAD_LOGGER_H
