// ----------------------------------------------------
// プレイヤーカメラ制御 [player_camera.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#ifndef PLAYER_CAMERA_H
#define PLAYER_CAMERA_H

#include <DirectXMath.h>

void PlayerCamera_Initialize();
void PlayerCamera_Finalize();

void PlayerCamera_Update(double elapsed_time);

const DirectX::XMFLOAT3& PlayerCamera_GetFront();
const DirectX::XMFLOAT3& PlayerCamera_GetPosition();

const DirectX::XMFLOAT4X4& PlayerCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& PlayerCamera_GetPerspectiveMatrix();

void PlayerCamera_Shake(float shake);
void PlayerCamera_Shake_position(float shake);

#endif // PLAYER_CAMERA_H
