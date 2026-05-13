// ----------------------------------------------------
// ゲームパッド入力 [gamepad.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-06-28
// Version: 1.0
// ----------------------------------------------------
#ifndef GAMEPAD_H
#define GAMEPAD_H

#include <Windows.h>

// ゲームパッドボタン定義
enum GamepadButton
{
    GP_BUTTON_A = 0,
    GP_BUTTON_B,
    GP_BUTTON_X,
    GP_BUTTON_Y,
    GP_BUTTON_LB,
    GP_BUTTON_RB,
    GP_BUTTON_BACK,
    GP_BUTTON_START,
    GP_BUTTON_LSTICK,
    GP_BUTTON_RSTICK,
    GP_BUTTON_DPAD_UP,
    GP_BUTTON_DPAD_DOWN,
    GP_BUTTON_DPAD_LEFT,
    GP_BUTTON_DPAD_RIGHT,
    GP_BUTTON_COUNT
};

// ゲームパッド状態構造体
struct GamepadState
{
    bool connected;           // 接続されているか
    
    // ボタン状態
    bool buttons[GP_BUTTON_COUNT];
  
    // アナログスティック (-1.0 ~ 1.0)
    float leftStickX;
    float leftStickY;
    float rightStickX;
    float rightStickY;
    
    // トリガー (0.0 ~ 1.0)
    float leftTrigger;
    float rightTrigger;
};


void Gamepad_Initialize();
void Gamepad_Finalize();
void Gamepad_Update();

// 状態取得
bool Gamepad_IsConnected(int playerIndex = 0);
const GamepadState* Gamepad_GetState(int playerIndex = 0);

// ボタン入力
bool Gamepad_IsPressed(GamepadButton button, int playerIndex = 0);
bool Gamepad_IsTrigger(GamepadButton button, int playerIndex = 0);
bool Gamepad_IsRelease(GamepadButton button, int playerIndex = 0);

// アナログ入力
float Gamepad_GetLeftStickX(int playerIndex = 0);
float Gamepad_GetLeftStickY(int playerIndex = 0);
float Gamepad_GetRightStickX(int playerIndex = 0);
float Gamepad_GetRightStickY(int playerIndex = 0);
float Gamepad_GetLeftTrigger(int playerIndex = 0);
float Gamepad_GetRightTrigger(int playerIndex = 0);

// ぶるぶる
void Gamepad_SetVibration(float leftMotor, float rightMotor, int playerIndex = 0);
void Gamepad_StopVibration(int playerIndex = 0);

#endif // GAMEPAD_H
