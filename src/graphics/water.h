// ----------------------------------------------------
// 水面処理 [water.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-22
// Version: 1.0
// ----------------------------------------------------
#ifndef WATER_H
#define WATER_H

#include <d3d11.h>
#include <DirectXMath.h>

void Water_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Water_Finalize();

void Water_Update(double elapsed_time);
void Water_Draw();

void Water_SetCameraPosition(const DirectX::XMFLOAT3& cameraPos);

// 反射ビュー投影行列を設定（頂点シェーダで反射UV生成に使用）
void Water_SetReflectionMatrix(const DirectX::XMFLOAT4X4& viewProjReflected);

// ライトビュープロジェクション行列を設定
void Water_SetLightMatrix(const DirectX::XMFLOAT4X4& lightViewProj);

#endif // WATER_H
