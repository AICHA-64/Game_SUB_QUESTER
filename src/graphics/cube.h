// ----------------------------------------------------
// 3Dキューブの表示 [cube.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-09
// Version: 1.0
// ----------------------------------------------------
#ifndef CUBE_H
#define CUBE_H

#include <d3d11.h>
#include <DirectXMath.h>
#include "collision.h"

void Cube_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Cube_Finalize(void);
void Cube_Draw(int texId, const DirectX::XMMATRIX& mtxWorld);
void Cube_Draw_Minimal(const DirectX::XMMATRIX& mtxWorld);
void Cube_DrawShadow(const DirectX::XMMATRIX& mtxWorld); // シャドウマップ専用描画関数（深度のみ）
AABB Cube_GetAABB(const DirectX::XMFLOAT3& position);

#endif // CUBE_H
