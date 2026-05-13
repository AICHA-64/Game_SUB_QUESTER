// ----------------------------------------------------
// デバッグ描画 [debug_draw.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#ifndef DEBUG_DRAW_H
#define DEBUG_DRAW_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"

// 初期化・終了
void DebugDraw_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void DebugDraw_Finalize();

// ミニマップ描画モードの設定/取得
void DebugDraw_SetMinimapMode(bool isMinimapMode);
bool DebugDraw_IsMinimapMode();

// AABBをワイヤーフレーム描画
void DebugDraw_AABB(const AABB& aabb, const DirectX::XMFLOAT4& color);

// Sphereをワイヤーフレーム描画
void DebugDraw_Sphere(const Sphere& sphere, const DirectX::XMFLOAT4& color);

#endif // DEBUG_DRAW_H
