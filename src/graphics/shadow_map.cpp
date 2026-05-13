// ----------------------------------------------------
// シャドウマップ [shadow_map.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-17
// Version: 1.0
// ----------------------------------------------------
#include "shadow_map.h"
#include "direct3d.h"

ShadowMap::ShadowMap(ID3D11Device* device, int width, int height)
    : m_Width(width), m_Height(height), m_pShaderResourceView(nullptr), m_pDepthStencilView(nullptr)
{
    // 1. テクスチャリソースの作成
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_Width;
    texDesc.Height = m_Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // DSVとSRVで異なるフォーマットを使うためのTYPELESS
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    ID3D11Texture2D* depthMap = nullptr;
    device->CreateTexture2D(&texDesc, nullptr, &depthMap);

    // 2. Depth Stencil View (DSV) の作成（書き込み用）
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = 0;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 深度フォーマット
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(depthMap, &dsvDesc, &m_pDepthStencilView);

    // 3. Shader Resource View (SRV) の作成（読み込み用）
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // 赤成分(R)として深度を読み取る
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = texDesc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    device->CreateShaderResourceView(depthMap, &srvDesc, &m_pShaderResourceView);

    // テクスチャリソースの参照カウンタを減らす（Viewが参照を持っているので解放してOK）
    if (depthMap) depthMap->Release();

    // 4. ビューポートの設定
    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(width);
    m_Viewport.Height = static_cast<float>(height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;
}

ShadowMap::~ShadowMap()
{
    if (m_pShaderResourceView) m_pShaderResourceView->Release();
    if (m_pDepthStencilView) m_pDepthStencilView->Release();
}

void ShadowMap::BindDsvAndSetNullRenderTarget(ID3D11DeviceContext* deviceContext)
{
    // ビューポートをセット
    deviceContext->RSSetViewports(1, &m_Viewport);

    // レンダーターゲットをNULLにし、深度バッファのみをセットする
    // (カラー出力は不要なため、負荷を減らす)
    ID3D11RenderTargetView* nullRenderTarget[1] = { nullptr };
    deviceContext->OMSetRenderTargets(1, nullRenderTarget, m_pDepthStencilView);

    // ピクセルシェーダーを無効化して深度のみ描画（D3D11警告を回避）
    deviceContext->PSSetShader(nullptr, nullptr, 0);

    // 深度バッファをクリア
    deviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}
