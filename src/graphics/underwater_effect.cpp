// ----------------------------------------------------
// 水中エフェクト [underwater_effect.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-01-10
// Version: 1.0
// ----------------------------------------------------
#include "underwater_effect.h"
#include "direct3d.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <cstring>
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

// 定数バッファ構造体
struct UnderwaterConstantBuffer {
    XMFLOAT4 color;        // 水中の色
    float depth;           // 深度の強度
    float padding[3];      // パディング
};

// グローバル変数
static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Buffer* g_pConstantBuffer = nullptr;
static ID3D11VertexShader* g_pVertexShader = nullptr;
static ID3D11PixelShader* g_pPixelShader = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;
static ID3D11BlendState* g_pBlendState = nullptr;

static float g_WaterSurfaceY = -1.0f;      // 水面のY座標
static float g_MaxDepth = 50.0f;          // 最大深度
static XMFLOAT4 g_WaterColor = XMFLOAT4(0.0f, 0.3f, 0.5f, 0.0f); // 水中の色（青緑色）

// 頂点構造体
struct UnderwaterVertex {
    XMFLOAT3 position;
    XMFLOAT2 texcoord;
};

// シェーダーコード
static const char* g_VertexShaderCode = R"(
struct VS_INPUT {
    float4 position : POSITION;
    float2 texcoord : TEXCOORD0;
};

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    output.position = input.position;
    output.texcoord = input.texcoord;
    return output;
}
)";

static const char* g_PixelShaderCode = R"(
cbuffer UnderwaterBuffer : register(b0) {
    float4 color;
    float depth;
    float3 padding;
};

struct PS_INPUT {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    // 深度に応じて色の強度を調整
    float4 finalColor = color;
    finalColor.a = depth;
    return finalColor;
}
)";

// シェーダーコンパイル関数
static HRESULT CompileShaderFromMemory(const char* source, const char* entryPoint, 
                                       const char* target, ID3DBlob** ppBlob) {
    ID3DBlob* pErrorBlob = nullptr;
    DWORD flags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    flags |= D3DCOMPILE_DEBUG;
#endif

    HRESULT hr = D3DCompile(
        source, lstrlenA(source), nullptr, nullptr, nullptr,
        entryPoint, target, flags, 0, ppBlob, &pErrorBlob
    );

    if (FAILED(hr)) {
        if (pErrorBlob) {
#ifdef _DEBUG
            OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
#endif
            pErrorBlob->Release();
        }
        return hr;
    }

    if (pErrorBlob) {
        pErrorBlob->Release();
    }

    return S_OK;
}

void UnderwaterEffect_Initialize() {
    ID3D11Device* pDevice = Direct3D_GetDevice();
    if (!pDevice) return;

    // 頂点バッファの作成
    UnderwaterVertex vertices[] = {
        { XMFLOAT3(-1.0f,  1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) },
        { XMFLOAT3( 1.0f,  1.0f, 0.0f), XMFLOAT2(1.0f, 0.0f) },
        { XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) },
        { XMFLOAT3( 1.0f, -1.0f, 0.0f), XMFLOAT2(1.0f, 1.0f) },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = vertices;

    HRESULT hr = pDevice->CreateBuffer(&bd, &initData, &g_pVertexBuffer);
    if (FAILED(hr)) return;

    // 定数バッファの作成
    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.ByteWidth = sizeof(UnderwaterConstantBuffer);
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    hr = pDevice->CreateBuffer(&cbd, nullptr, &g_pConstantBuffer);
    if (FAILED(hr)) return;

    // 頂点シェーダーのコンパイルと作成
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromMemory(g_VertexShaderCode, "main", "vs_5_0", &pVSBlob);
    if (SUCCEEDED(hr)) {
        hr = pDevice->CreateVertexShader(
            pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(),
            nullptr,
            &g_pVertexShader
        );

        if (SUCCEEDED(hr)) {
            // 入力レイアウトの作成
            D3D11_INPUT_ELEMENT_DESC layout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            };

            hr = pDevice->CreateInputLayout(
                layout,
                ARRAYSIZE(layout),
                pVSBlob->GetBufferPointer(),
                pVSBlob->GetBufferSize(),
                &g_pInputLayout
            );
        }

        pVSBlob->Release();
    }

    // ピクセルシェーダーのコンパイルと作成
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromMemory(g_PixelShaderCode, "main", "ps_5_0", &pPSBlob);
    if (SUCCEEDED(hr)) {
        hr = pDevice->CreatePixelShader(
            pPSBlob->GetBufferPointer(),
            pPSBlob->GetBufferSize(),
            nullptr,
            &g_pPixelShader
        );
        pPSBlob->Release();
    }

    // ブレンドステートの作成（アルファブレンド）
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    hr = pDevice->CreateBlendState(&blendDesc, &g_pBlendState);
}

void UnderwaterEffect_Finalize() {
    if (g_pBlendState) {
        g_pBlendState->Release();
        g_pBlendState = nullptr;
    }
    if (g_pInputLayout) {
        g_pInputLayout->Release();
        g_pInputLayout = nullptr;
    }
    if (g_pPixelShader) {
        g_pPixelShader->Release();
        g_pPixelShader = nullptr;
    }
    if (g_pVertexShader) {
        g_pVertexShader->Release();
        g_pVertexShader = nullptr;
    }
    if (g_pConstantBuffer) {
        g_pConstantBuffer->Release();
        g_pConstantBuffer = nullptr;
    }
    if (g_pVertexBuffer) {
        g_pVertexBuffer->Release();
        g_pVertexBuffer = nullptr;
    }
}

void UnderwaterEffect_Update(float playerY) {
    // プレイヤーが水面より下にいるかチェック
    if (playerY < g_WaterSurfaceY) {
        // 水中にいる場合、深度を計算
        float depth = g_WaterSurfaceY - playerY;

        float normalizedDepth = (std::min)(depth / g_MaxDepth, 1.0f);
        
        // 色の透明度を深度に応じて調整

        g_WaterColor.w = 0.4f + normalizedDepth * 0.45f;
    } else {
        // 水面より上にいる場合、透明度を0にする
        g_WaterColor.w = 0.0f;
    }
}

void UnderwaterEffect_Draw() {
    // 透明度が0の場合は描画しない
    if (g_WaterColor.w <= 0.0f) return;

    ID3D11DeviceContext* pContext = Direct3D_GetContext();
    if (!pContext) return;

    // 定数バッファを更新
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = pContext->Map(g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        UnderwaterConstantBuffer* pCB = (UnderwaterConstantBuffer*)mappedResource.pData;
        pCB->color = g_WaterColor;
        pCB->depth = g_WaterColor.w;
        pContext->Unmap(g_pConstantBuffer, 0);
    }

    // 以前のステートを保存
    ID3D11BlendState* pPrevBlendState = nullptr;
    FLOAT prevBlendFactor[4];
    UINT prevSampleMask;
    pContext->OMGetBlendState(&pPrevBlendState, prevBlendFactor, &prevSampleMask);

    ID3D11DepthStencilState* pPrevDepthState = nullptr;
    UINT prevStencilRef;
    pContext->OMGetDepthStencilState(&pPrevDepthState, &prevStencilRef);

    // 描画ステートの設定
    UINT stride = sizeof(UnderwaterVertex);
    UINT offset = 0;
    pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    pContext->IASetInputLayout(g_pInputLayout);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    pContext->VSSetShader(g_pVertexShader, nullptr, 0);
    pContext->PSSetShader(g_pPixelShader, nullptr, 0);
    pContext->PSSetConstantBuffers(0, 1, &g_pConstantBuffer);

    // ブレンドステートを設定
    FLOAT blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    pContext->OMSetBlendState(g_pBlendState, blendFactor, 0xffffffff);

    ID3D11DepthStencilState* pDepthState = nullptr;
    D3D11_DEPTH_STENCIL_DESC depthDesc = {};
    depthDesc.DepthEnable = TRUE;
    depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    
    ID3D11Device* pDevice = Direct3D_GetDevice();
    pDevice->CreateDepthStencilState(&depthDesc, &pDepthState);
    pContext->OMSetDepthStencilState(pDepthState, 0);

    pContext->Draw(4, 0);

    pContext->OMSetBlendState(pPrevBlendState, prevBlendFactor, prevSampleMask);
    pContext->OMSetDepthStencilState(pPrevDepthState, prevStencilRef);

    if (pPrevBlendState) pPrevBlendState->Release();
    if (pPrevDepthState) pPrevDepthState->Release();
    if (pDepthState) pDepthState->Release();
}

float UnderwaterEffect_GetWaterSurfaceY() {
    return g_WaterSurfaceY;
}
