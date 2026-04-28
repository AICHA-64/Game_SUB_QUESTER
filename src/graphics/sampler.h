// ----------------------------------------------------
// サンプラーの設定ユーティリティー [sampler.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-09
// Version: 1.0
// ----------------------------------------------------
#ifndef SAMPLER_H
#define SAMPLER_H

#include <d3d11.h>
#include <DirectXMath.h>

void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Sampler_Finalize(void);

void Sampler_SetFilterPoint();
void Sampler_SetFilterLinear();
void Sampler_SetFilterAnisotropic();

void Sampler_CreateShadowSampler(ID3D11Device* pDevice);
void Sampler_SetShadowSampler(ID3D11DeviceContext* pContext);

#endif // SAMPLER_H
