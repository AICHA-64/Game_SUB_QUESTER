// ----------------------------------------------------
// 水面反射処理 [water_reflect.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-24
// Version: 1.0
// ----------------------------------------------------
#ifndef WATER_REFLECT_H
#define WATER_REFLECT_H

#include <d3d11.h>

struct WaterRT
{
    ID3D11Texture2D* pReflectionTex = nullptr;
    ID3D11RenderTargetView* pReflectionRTV = nullptr;
    ID3D11ShaderResourceView* pReflectionSRV = nullptr;

    ID3D11Texture2D* pSceneColorTex = nullptr;
    ID3D11RenderTargetView* pSceneColorRTV = nullptr;
    ID3D11ShaderResourceView* pSceneColorSRV = nullptr;


    ID3D11Texture2D* pReflectionDepthTex = nullptr;
    ID3D11DepthStencilView* pReflectionDSV = nullptr;
};

bool WaterRT_Create(ID3D11Device* device, int width, int height, WaterRT& out);
void WaterRT_Release(WaterRT& rt);

#endif // WATER_REFLECT_H
