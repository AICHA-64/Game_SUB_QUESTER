// ----------------------------------------------------
// メッシュフィールドの表示 [meshfield.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-19
// Version: 1.0
// ----------------------------------------------------
#ifndef MESHFIELD_H
#define MESHFIELD_H

#include <d3d11.h>
#include <DirectXMath.h>

void Meshfield_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Meshfield_Finalize(void);
void Meshfield_Draw(void);

#endif // MESHFIELD_H
