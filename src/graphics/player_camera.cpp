// ----------------------------------------------------
// プレイヤーカメラ制御 [player_camera.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------

#include "player_camera.h"

#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include <sstream>
#include "player.h"
#include "mouse.h"
#include "key_logger.h"
#include "gamepad.h"

static XMFLOAT3 g_PlayerCameraFront = { 0.0f, 0.0f, 1.0f };
static XMFLOAT3 g_PlayerCameraPosition = { 0.0f, 0.0f, 0.0f };

static XMFLOAT4X4 g_PlayerCameraMatrix{};
static XMFLOAT4X4 g_PlayerCameraPerspectiveMatrix{};

static constexpr float CAMERA_ROTATION_SPEED = XMConvertToRadians(60);

static float g_CameraShake = 0.0f;
static double g_CameraShakeTimer = 0.0;

static float g_CameraShake_Position = 0.0f;
static double g_CameraShakeTimer_Position = 0.0;

static float g_Fov = XMConvertToRadians(60);

// 注視点までの距離
static constexpr float TARGET_DISTANCE = 50.0f;

// カメラ回転の最大角度（上下）
static constexpr float PITCH_RANGE = XM_PIDIV2 * 0.8f; // 上下限界: ±72度

// キーボード/ゲームパッドによるピッチオフセット
static float g_KeyboardPitchOffset = 0.0f;
// キーボードでのピッチ変化速度
static constexpr float KEYBOARD_PITCH_SPEED = 1.0f;

// カメラのYaw角度（左右回転）
static float g_CameraYaw = 0.0f;
// キーボード/ゲームパッドによる回転速度
static constexpr float KEYBOARD_YAW_SPEED = 1.0f;

// 初回 Update でのマウス相対位置を無視するフラグ
static bool g_PlayerCamera_FirstUpdate = false;

// 前フレームのマウス位置（相対移動計算用）
static float g_PrevMouseX = 0.0f;
static float g_PrevMouseY = 0.0f;

// マウス感度
static constexpr float MOUSE_SENSITIVITY_X = 0.003f;
static constexpr float MOUSE_SENSITIVITY_Y = 0.003f;

void PlayerCamera_Initialize()
{
	// マウス絶対座標モードに設定（カーソル位置を使用するため）
	Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
	
	// キーボードピッチオフセットを初期化
	g_KeyboardPitchOffset = 0.0f;
	
	// カメラYawをプレイヤーの初期向きに合わせる
	XMFLOAT3 playerFront = Player::GetInstance().GetFront();
	g_CameraYaw = atan2f(playerFront.x, playerFront.z);

	// 初回 Update でのマウス相対位置を無視するためフラグを立てる
	g_PlayerCamera_FirstUpdate = true;
	
	// 前フレームのマウス位置を初期化
	g_PrevMouseX = 0.0f;
	g_PrevMouseY = 0.0f;
}

void PlayerCamera_Finalize()
{
}

void PlayerCamera_Update(double elapsed_time)
{
	// カメラ位置を計算（プレイヤーの頭上）
	// 補間済みの描画座標を使用してジッターを抑える
	XMVECTOR playerPos = XMLoadFloat3(&Player::GetInstance().GetDrawPosition());
	
	// プレイヤーの高さに対して上方向にオフセット
	XMVECTOR position = playerPos + XMVectorSet(0.0f, 1.5f, 0.0f, 0.0f);
	
	XMStoreFloat3(&g_PlayerCameraPosition, position);

	// 画面サイズを取得
	float screenWidth = (float)Direct3D_GetBackBufferWidth();
	float screenHeight = (float)Direct3D_GetBackBufferHeight();
	
	// マウスの位置を取得
	Mouse_State mouseState;
	Mouse_GetState(&mouseState);
	
	// 初回フレームはマウス位置を記録するだけ
	if (g_PlayerCamera_FirstUpdate) {
		g_PlayerCamera_FirstUpdate = false;
		g_PrevMouseX = 0.0f;
		g_PrevMouseY = 0.0f;
	}
	
	// MOUSE_POSITION_MODE_RELATIVE の場合は mouseState.x と mouseState.y がそのまま差分
	float mouseDeltaX = (float)mouseState.x;
	float mouseDeltaY = (float)mouseState.y;
	
	// マウス移動量からカメラ回転を計算
	g_CameraYaw += mouseDeltaX * MOUSE_SENSITIVITY_X;
	g_KeyboardPitchOffset += mouseDeltaY * MOUSE_SENSITIVITY_Y;
	
	// キーボード入力による上下操作（累積）
	if (KeyLogger_IsPressed(KK_UP)) {
		g_KeyboardPitchOffset -= KEYBOARD_PITCH_SPEED * (float)elapsed_time;
	}
	if (KeyLogger_IsPressed(KK_DOWN)) {
		g_KeyboardPitchOffset += KEYBOARD_PITCH_SPEED * (float)elapsed_time;
	}
	
	// キーボード入力による左右操作（累積）
	if (KeyLogger_IsPressed(KK_LEFT)) {
		g_CameraYaw -= KEYBOARD_YAW_SPEED * (float)elapsed_time;
	}
	if (KeyLogger_IsPressed(KK_RIGHT)) {
		g_CameraYaw += KEYBOARD_YAW_SPEED * (float)elapsed_time;
	}
	
	// 右スティックY軸による上下操作（累積）
	float rightStickY = Gamepad_GetRightStickY();
	if (rightStickY != 0.0f) {
		// スティック上に倒すと上を向く（Y軸反転）
		g_KeyboardPitchOffset -= rightStickY * KEYBOARD_PITCH_SPEED * (float)elapsed_time;
	}
	
	// 右スティックX軸による左右操作（累積）
	float rightStickX = Gamepad_GetRightStickX();
	if (rightStickX != 0.0f) {
		g_CameraYaw += rightStickX * KEYBOARD_YAW_SPEED * (float)elapsed_time;
	}
	
	// キーボードオフセットを -1.0 ~ 1.0 に
	if (g_KeyboardPitchOffset < -1.0f) g_KeyboardPitchOffset = -1.0f;
	if (g_KeyboardPitchOffset > 1.0f) g_KeyboardPitchOffset = 1.0f;
	
	// Yaw角度を 0 ~ 2π に正規化
	while (g_CameraYaw > XM_2PI) g_CameraYaw -= XM_2PI;
	while (g_CameraYaw < 0.0f) g_CameraYaw += XM_2PI;
	
	// 正規化座標から回転角度を計算（上下のみ）
	float pitch = -g_KeyboardPitchOffset * PITCH_RANGE; // 上下限界: ±72度
	
	// 回転角度からカメラの前方ベクトルを計算
	XMVECTOR front = XMVectorSet(
		cosf(pitch) * sinf(g_CameraYaw),
		sinf(pitch),
		cosf(pitch) * cosf(g_CameraYaw),
		0.0f
	);
	front = XMVector3Normalize(front);
	
	XMStoreFloat3(&g_PlayerCameraFront, front);
	
	// 注視点を計算（カメラ位置 + 前方ベクトル * 距離）
	XMVECTOR target = position + front * TARGET_DISTANCE;


	// カメラシェイクの実装
	target = XMVectorSetY(target, XMVectorGetY(target) + static_cast<float>(g_CameraShake * cos(g_CameraShakeTimer)));
	g_CameraShakeTimer += XM_2PI / 3.0f * 60 * elapsed_time;
	g_CameraShake *= 0.9f; // 徐々に減衰

	// カメラシェイク(Position)
	position = XMVectorSetY(position, XMVectorGetY(position) + static_cast<float>(g_CameraShake_Position * cos(g_CameraShakeTimer_Position)));
	g_CameraShakeTimer_Position += XM_2PI / 3.0f * 60 * elapsed_time;
	g_CameraShake_Position *= 0.9f; // 徐々に減衰

	// カメラシェイク時はコントローラーも振動させる
	float totalShake = g_CameraShake + g_CameraShake_Position;
	if (totalShake > 0.01f) {
		// シェイクの強さに応じて振動（最大1.0）
		float vibrationIntensity = fminf(totalShake * 2.0f, 1.0f);
		Gamepad_SetVibration(vibrationIntensity, vibrationIntensity);
	}
	else {
		// シェイクが終わったら振動を停止
		Gamepad_StopVibration();
	}


	// ビュー座標変換行列の作成
	XMMATRIX mtxView = XMMatrixLookAtLH(
		position,
		target,
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

	XMStoreFloat4x4(&g_PlayerCameraMatrix, mtxView);

	// パースペクティブ行列
	float aspectRatio = screenWidth / screenHeight;
	float nearz = 0.1f;
	float farz = 1000.0f;
	XMMATRIX mtxPerspective = XMMatrixPerspectiveFovLH(g_Fov, aspectRatio, nearz, farz);
	XMStoreFloat4x4(&g_PlayerCameraPerspectiveMatrix, mtxPerspective);
}

const DirectX::XMFLOAT3& PlayerCamera_GetPosition()
{
	return g_PlayerCameraPosition;
}

const DirectX::XMFLOAT3& PlayerCamera_GetFront()
{
	return g_PlayerCameraFront;
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix()
{
	return g_PlayerCameraMatrix;
}

const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix()
{
	return g_PlayerCameraPerspectiveMatrix;
}

void PlayerCamera_Shake(float shake)
{
	g_CameraShake = shake;
	g_CameraShakeTimer = 0.0;
}

void PlayerCamera_Shake_position(float shake)
{
	g_CameraShake_Position = shake;
	g_CameraShakeTimer_Position = 0.0;
}
