// ----------------------------------------------------
// ゲームパッド入力 [gamepad.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-06-28
// Version: 1.0
// ----------------------------------------------------
#include "gamepad.h"
#include <Xinput.h>
#include <cmath>

#pragma comment(lib, "xinput.lib")

// 最大プレイヤー数
static constexpr int MAX_PLAYERS = 4;

// デッドゾーン設定
static constexpr float STICK_DEADZONE = 0.24f;// 約7849/32767
static constexpr float TRIGGER_DEADZONE = 0.12f; // 約30/255

// 現在と前フレームの状態
static GamepadState g_CurrentState[MAX_PLAYERS] = {};
static GamepadState g_PrevState[MAX_PLAYERS] = {};

// スティック値を正規化（デッドゾーン適用）
static float NormalizeStick(SHORT value, float deadzone)
{
    float normalized = value / 32767.0f;
    
    if (std::abs(normalized) < deadzone) {
        return 0.0f;
    }
    
    // デッドゾーン外の値を0?1にリマップ
    float sign = (normalized > 0) ? 1.0f : -1.0f;
    normalized = (std::abs(normalized) - deadzone) / (1.0f - deadzone);
    return normalized * sign;
}

// トリガー値を正規化
static float NormalizeTrigger(BYTE value, float deadzone)
{
    float normalized = value / 255.0f;
    
    if (normalized < deadzone) {
     return 0.0f;
    }
    
  return (normalized - deadzone) / (1.0f - deadzone);
}

void Gamepad_Initialize()
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        g_CurrentState[i] = {};
        g_PrevState[i] = {};
    }
}

void Gamepad_Finalize()
{
    // すべてのバイブレーションを停止
    for (int i = 0; i < MAX_PLAYERS; i++) {
        Gamepad_StopVibration(i);
    }
}

void Gamepad_Update()
{
    for (int i = 0; i < MAX_PLAYERS; i++) {
        // 前フレームの状態を保存
        g_PrevState[i] = g_CurrentState[i];
        
        // XInputの状態を取得
        XINPUT_STATE xinputState = {};
        DWORD result = XInputGetState(i, &xinputState);
        
        GamepadState& state = g_CurrentState[i];
        
        if (result == ERROR_SUCCESS) {
            state.connected = true;
          
            const XINPUT_GAMEPAD& gp = xinputState.Gamepad;
      
            // ボタン状態
            state.buttons[GP_BUTTON_A] = (gp.wButtons & XINPUT_GAMEPAD_A) != 0;
            state.buttons[GP_BUTTON_B] = (gp.wButtons & XINPUT_GAMEPAD_B) != 0;
            state.buttons[GP_BUTTON_X] = (gp.wButtons & XINPUT_GAMEPAD_X) != 0;
            state.buttons[GP_BUTTON_Y] = (gp.wButtons & XINPUT_GAMEPAD_Y) != 0;
            state.buttons[GP_BUTTON_LB] = (gp.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
            state.buttons[GP_BUTTON_RB] = (gp.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
            state.buttons[GP_BUTTON_BACK] = (gp.wButtons & XINPUT_GAMEPAD_BACK) != 0;
            state.buttons[GP_BUTTON_START] = (gp.wButtons & XINPUT_GAMEPAD_START) != 0;
            state.buttons[GP_BUTTON_LSTICK] = (gp.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0;
            state.buttons[GP_BUTTON_RSTICK] = (gp.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0;
            state.buttons[GP_BUTTON_DPAD_UP] = (gp.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
            state.buttons[GP_BUTTON_DPAD_DOWN] = (gp.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
            state.buttons[GP_BUTTON_DPAD_LEFT] = (gp.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
            state.buttons[GP_BUTTON_DPAD_RIGHT] = (gp.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
    
            // アナログスティック
            state.leftStickX = NormalizeStick(gp.sThumbLX, STICK_DEADZONE);
            state.leftStickY = NormalizeStick(gp.sThumbLY, STICK_DEADZONE);
            state.rightStickX = NormalizeStick(gp.sThumbRX, STICK_DEADZONE);
            state.rightStickY = NormalizeStick(gp.sThumbRY, STICK_DEADZONE);
    
            // トリガー
            state.leftTrigger = NormalizeTrigger(gp.bLeftTrigger, TRIGGER_DEADZONE);
            state.rightTrigger = NormalizeTrigger(gp.bRightTrigger, TRIGGER_DEADZONE);
        }
        else {
            // 接続されていない
            state = {};
            state.connected = false;
        }
    }
}

bool Gamepad_IsConnected(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    return g_CurrentState[playerIndex].connected;
}

const GamepadState* Gamepad_GetState(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return nullptr;
    return &g_CurrentState[playerIndex];
}

bool Gamepad_IsPressed(GamepadButton button, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    if (button < 0 || button >= GP_BUTTON_COUNT) return false;
    return g_CurrentState[playerIndex].buttons[button];
}

bool Gamepad_IsTrigger(GamepadButton button, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    if (button < 0 || button >= GP_BUTTON_COUNT) return false;
    return g_CurrentState[playerIndex].buttons[button] && !g_PrevState[playerIndex].buttons[button];
}

bool Gamepad_IsRelease(GamepadButton button, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return false;
    if (button < 0 || button >= GP_BUTTON_COUNT) return false;
    return !g_CurrentState[playerIndex].buttons[button] && g_PrevState[playerIndex].buttons[button];
}

float Gamepad_GetLeftStickX(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
    return g_CurrentState[playerIndex].leftStickX;
}

float Gamepad_GetLeftStickY(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
    return g_CurrentState[playerIndex].leftStickY;
}

float Gamepad_GetRightStickX(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
    return g_CurrentState[playerIndex].rightStickX;
}

float Gamepad_GetRightStickY(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
    return g_CurrentState[playerIndex].rightStickY;
}

float Gamepad_GetLeftTrigger(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
    return g_CurrentState[playerIndex].leftTrigger;
}

float Gamepad_GetRightTrigger(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return 0.0f;
  return g_CurrentState[playerIndex].rightTrigger;
}

void Gamepad_SetVibration(float leftMotor, float rightMotor, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) return;
    
    XINPUT_VIBRATION vibration = {};
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);
    XInputSetState(playerIndex, &vibration);
}

void Gamepad_StopVibration(int playerIndex)
{
    Gamepad_SetVibration(0.0f, 0.0f, playerIndex);
}
