// ----------------------------------------------------
// マップ表示用カメラ [map_camera.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-08
// Version: 1.0
// ----------------------------------------------------
#ifndef MAP_CAMERA_H
#define MAP_CAMERA_H

#include <DirectXMath.h>

void MapCamera_Initialize();
void MapCamera_Finalize();

void MapCamera_SetPosition(const DirectX::XMFLOAT3& position);
void MapCamera_SetFront(const DirectX::XMFLOAT3& front);

const DirectX::XMFLOAT4X4& MapCamera_GetViewMatrix();
const DirectX::XMFLOAT4X4& MapCamera_GetProjectionMatrix();


#endif // MAP_CAMERA_H
