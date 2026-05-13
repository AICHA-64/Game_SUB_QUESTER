// ----------------------------------------------------
// 水面反射処理 [water_reflect.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-24
// Version: 1.0
// ----------------------------------------------------
#include "water_reflect.h"
#include <d3d11.h>
#include <dxgiformat.h>

template<typename T>
static void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

bool WaterRT_Create(ID3D11Device* device, int width, int height, WaterRT& out)
{
    if (!device || width <= 0 || height <= 0) return false;

    // 既存を掃除
    WaterRT_Release(out);

    // 反射カラーRT
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = static_cast<UINT>(width);
    texDesc.Height = static_cast<UINT>(height);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &out.pReflectionTex);
    if (FAILED(hr)) { WaterRT_Release(out); return false; }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = texDesc.Format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    hr = device->CreateRenderTargetView(out.pReflectionTex, &rtvDesc, &out.pReflectionRTV);
    if (FAILED(hr)) { WaterRT_Release(out); return false; }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = texDesc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    hr = device->CreateShaderResourceView(out.pReflectionTex, &srvDesc, &out.pReflectionSRV);
    if (FAILED(hr)) { WaterRT_Release(out); return false; }

    // 深度ステンシル (反射パス専用)
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = static_cast<UINT>(width);
    depthDesc.Height = static_cast<UINT>(height);
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = device->CreateTexture2D(&depthDesc, nullptr, &out.pReflectionDepthTex);
    if (FAILED(hr)) { WaterRT_Release(out); return false; }

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = depthDesc.Format;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = device->CreateDepthStencilView(out.pReflectionDepthTex, &dsvDesc, &out.pReflectionDSV);
    if (FAILED(hr)) { WaterRT_Release(out); return false; }

    return true;
}

void WaterRT_Release(WaterRT& rt)
{
    SafeRelease(rt.pReflectionDSV);
    SafeRelease(rt.pReflectionDepthTex);

    SafeRelease(rt.pReflectionSRV);
    SafeRelease(rt.pReflectionRTV);
    SafeRelease(rt.pReflectionTex);

    SafeRelease(rt.pSceneColorSRV);
    SafeRelease(rt.pSceneColorRTV);
    SafeRelease(rt.pSceneColorTex);
}
