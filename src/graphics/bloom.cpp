// ----------------------------------------------------
// Bloomエフェクト [bloom.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-15
// Version: 1.0 (Multi-stage Bloom)
// ----------------------------------------------------
#include "bloom.h"
#include "direct3d.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib, "d3dcompiler.lib")

// Bloom用のレンダーターゲット（3段階: 1/2, 1/4, 1/8）
// Stage 1: 1/2解像度
static ID3D11Texture2D* g_pBrightTexture_1 = nullptr;
static ID3D11RenderTargetView* g_pBrightRTV_1 = nullptr;
static ID3D11ShaderResourceView* g_pBrightSRV_1 = nullptr;

static ID3D11Texture2D* g_pBlurH_Texture_1 = nullptr;
static ID3D11RenderTargetView* g_pBlurH_RTV_1 = nullptr;
static ID3D11ShaderResourceView* g_pBlurH_SRV_1 = nullptr;

static ID3D11Texture2D* g_pBlurV_Texture_1 = nullptr;
static ID3D11RenderTargetView* g_pBlurV_RTV_1 = nullptr;
static ID3D11ShaderResourceView* g_pBlurV_SRV_1 = nullptr;

// Stage 2: 1/4解像度
static ID3D11Texture2D* g_pBrightTexture_2 = nullptr;
static ID3D11RenderTargetView* g_pBrightRTV_2 = nullptr;
static ID3D11ShaderResourceView* g_pBrightSRV_2 = nullptr;

static ID3D11Texture2D* g_pBlurH_Texture_2 = nullptr;
static ID3D11RenderTargetView* g_pBlurH_RTV_2 = nullptr;
static ID3D11ShaderResourceView* g_pBlurH_SRV_2 = nullptr;

static ID3D11Texture2D* g_pBlurV_Texture_2 = nullptr;
static ID3D11RenderTargetView* g_pBlurV_RTV_2 = nullptr;
static ID3D11ShaderResourceView* g_pBlurV_SRV_2 = nullptr;

// Stage 3: 1/8解像度
static ID3D11Texture2D* g_pBrightTexture_3 = nullptr;
static ID3D11RenderTargetView* g_pBrightRTV_3 = nullptr;
static ID3D11ShaderResourceView* g_pBrightSRV_3 = nullptr;

static ID3D11Texture2D* g_pBlurH_Texture_3 = nullptr;
static ID3D11RenderTargetView* g_pBlurH_RTV_3 = nullptr;
static ID3D11ShaderResourceView* g_pBlurH_SRV_3 = nullptr;

static ID3D11Texture2D* g_pBlurV_Texture_3 = nullptr;
static ID3D11RenderTargetView* g_pBlurV_RTV_3 = nullptr;
static ID3D11ShaderResourceView* g_pBlurV_SRV_3 = nullptr;

// シェーダー
static ID3D11VertexShader* g_pVS = nullptr;
static ID3D11PixelShader* g_pPS_BrightPass = nullptr;
static ID3D11PixelShader* g_pPS_Downsample = nullptr;
static ID3D11PixelShader* g_pPS_BlurH = nullptr;
static ID3D11PixelShader* g_pPS_BlurV = nullptr;
static ID3D11PixelShader* g_pPS_Composite = nullptr;

// 入力レイアウト
static ID3D11InputLayout* g_pInputLayout = nullptr;

// フルスクリーン三角形用のバッファ（頂点バッファ不要のテクニック使用）

// 定数バッファ
static ID3D11Buffer* g_pCBBloom = nullptr;

// サンプラーステート
static ID3D11SamplerState* g_pSamplerLinear = nullptr;

// Bloomパラメータ（各層ごとに個別設定可能）
struct LayerParams {
    float threshold;
    float intensity;
};

static LayerParams g_LayerParams[3] = {
    { 0.1f, 0.6f },  // Stage 1: デフォルト（通常のBloom）
    { 0.8f, 1.6f },  // Stage 2: ハイライト層（強い光のみ）
    { 0.4f, 0.8f }   // Stage 3: 中間層
};

struct CB_Bloom {
    float threshold;
    float intensity;
    float padding[2];
};

static int g_Width = 0;
static int g_Height = 0;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

// シェーダーコンパイル
static HRESULT CompileShaderFromMemory(const char* shaderCode, const char* entryPoint, const char* profile, ID3DBlob** ppBlob)
{
    ID3DBlob* pErrorBlob = nullptr;
    HRESULT hr = D3DCompile(shaderCode, strlen(shaderCode), nullptr, nullptr, nullptr,
        entryPoint, profile, D3DCOMPILE_ENABLE_STRICTNESS, 0, ppBlob, &pErrorBlob);
    if (FAILED(hr) && pErrorBlob) {
#ifdef _DEBUG
        OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
#endif
        pErrorBlob->Release();
    }
    return hr;
}

// シェーダーコード（いったんインライン）
static const char* g_VSCode = R"(
struct VS_INPUT {
 uint vertexID : SV_VertexID;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input) {
    PS_INPUT output;
    // フルスクリーン三角形を生成
    float2 texcoord = float2((input.vertexID << 1) & 2, input.vertexID & 2);
    output.tex = texcoord;
    output.pos = float4(texcoord * float2(2, -2) + float2(-1, 1), 0, 1);
    return output;
}
)";

// 輝度抽出ピクセルシェーダー
static const char* g_PSBrightPassCode = R"(
Texture2D sceneTexture : register(t0);
SamplerState samplerLinear : register(s0);

cbuffer CBBloom : register(b0) {
    float threshold;
    float intensity;
    float2 padding;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 color = sceneTexture.Sample(samplerLinear, input.tex);
    float brightness = dot(color.rgb, float3(0.2126, 0.7152, 0.0722));
    if (brightness > threshold) {
   return color;
    }
    return float4(0, 0, 0, 1);
}
)";

// ダウンサンプリングシェーダー
static const char* g_PSDownsampleCode = R"(
Texture2D inputTexture : register(t0);
SamplerState samplerLinear : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    return inputTexture.Sample(samplerLinear, input.tex);
}
)";

// 横方向ブラー
static const char* g_PSBlurHCode = R"(
Texture2D inputTexture : register(t0);
SamplerState samplerLinear : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

static const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 main(PS_INPUT input) : SV_TARGET {
    float2 texSize;
    inputTexture.GetDimensions(texSize.x, texSize.y);
    float2 texelSize = 1.0 / texSize;
    
    float4 result = inputTexture.Sample(samplerLinear, input.tex) * weights[0];
    for(int i = 1; i < 5; i++) {
        result += inputTexture.Sample(samplerLinear, input.tex + float2(texelSize.x * i, 0)) * weights[i];
        result += inputTexture.Sample(samplerLinear, input.tex - float2(texelSize.x * i, 0)) * weights[i];
    }
    return result;
}
)";

// 縦方向ブラー
static const char* g_PSBlurVCode = R"(
Texture2D inputTexture : register(t0);
SamplerState samplerLinear : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

static const float weights[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };

float4 main(PS_INPUT input) : SV_TARGET {
    float2 texSize;
    inputTexture.GetDimensions(texSize.x, texSize.y);
    float2 texelSize = 1.0 / texSize;
    
 float4 result = inputTexture.Sample(samplerLinear, input.tex) * weights[0];
    for(int i = 1; i < 5; i++) {
        result += inputTexture.Sample(samplerLinear, input.tex + float2(0, texelSize.y * i)) * weights[i];
        result += inputTexture.Sample(samplerLinear, input.tex - float2(0, texelSize.y * i)) * weights[i];
    }
    return result;
}
)";

// 合成シェーダー（各層を個別の強度で合成）
static const char* g_PSCompositeCode = R"(
Texture2D sceneTexture : register(t0);
Texture2D bloomTexture1 : register(t1);
Texture2D bloomTexture2 : register(t2);
Texture2D bloomTexture3 : register(t3);
SamplerState samplerLinear : register(s0);

cbuffer CBBloom : register(b0) {
    float threshold;
    float intensity;
 float2 padding;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float4 scene = sceneTexture.Sample(samplerLinear, input.tex);
    float4 bloom1 = bloomTexture1.Sample(samplerLinear, input.tex);
    float4 bloom2 = bloomTexture2.Sample(samplerLinear, input.tex);
    float4 bloom3 = bloomTexture3.Sample(samplerLinear, input.tex);
    
    float4 bloomTotal = bloom1 * 0.4 + bloom2 * 1.2 + bloom3 * 0.6;
    return scene + bloomTotal * intensity;
}
)";

// テクスチャセットを作成
static void CreateBloomTextures(int width, int height,
    ID3D11Texture2D** ppBrightTex, ID3D11RenderTargetView** ppBrightRTV, ID3D11ShaderResourceView** ppBrightSRV,
    ID3D11Texture2D** ppBlurHTex, ID3D11RenderTargetView** ppBlurHRTV, ID3D11ShaderResourceView** ppBlurHSRV,
    ID3D11Texture2D** ppBlurVTex, ID3D11RenderTargetView** ppBlurVRTV, ID3D11ShaderResourceView** ppBlurVSRV)
{
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    // 輝度抽出用
    if (SUCCEEDED(g_pDevice->CreateTexture2D(&texDesc, nullptr, ppBrightTex))) {
        g_pDevice->CreateRenderTargetView(*ppBrightTex, nullptr, ppBrightRTV);
        g_pDevice->CreateShaderResourceView(*ppBrightTex, nullptr, ppBrightSRV);
    }

    // 横ブラー用
    if (SUCCEEDED(g_pDevice->CreateTexture2D(&texDesc, nullptr, ppBlurHTex))) {
        g_pDevice->CreateRenderTargetView(*ppBlurHTex, nullptr, ppBlurHRTV);
        g_pDevice->CreateShaderResourceView(*ppBlurHTex, nullptr, ppBlurHSRV);
    }

    // 縦ブラー用
    if (SUCCEEDED(g_pDevice->CreateTexture2D(&texDesc, nullptr, ppBlurVTex))) {
        g_pDevice->CreateRenderTargetView(*ppBlurVTex, nullptr, ppBlurVRTV);
        g_pDevice->CreateShaderResourceView(*ppBlurVTex, nullptr, ppBlurVSRV);
    }
}

void Bloom_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height)
{
    g_pDevice = pDevice;
    g_pContext = pContext;
    g_Width = width;
    g_Height = height;

    // HRESULT hr;

    // 3段階のブルーム用テクスチャを作成
    // Stage 1: 1/2解像度
    CreateBloomTextures(width / 2, height / 2,
        &g_pBrightTexture_1, &g_pBrightRTV_1, &g_pBrightSRV_1,
        &g_pBlurH_Texture_1, &g_pBlurH_RTV_1, &g_pBlurH_SRV_1,
        &g_pBlurV_Texture_1, &g_pBlurV_RTV_1, &g_pBlurV_SRV_1);

    // Stage 2: 1/4解像度
    CreateBloomTextures(width / 4, height / 4,
        &g_pBrightTexture_2, &g_pBrightRTV_2, &g_pBrightSRV_2,
        &g_pBlurH_Texture_2, &g_pBlurH_RTV_2, &g_pBlurH_SRV_2,
        &g_pBlurV_Texture_2, &g_pBlurV_RTV_2, &g_pBlurV_SRV_2);

    // Stage 3: 1/8解像度
    CreateBloomTextures(width / 8, height / 8,
        &g_pBrightTexture_3, &g_pBrightRTV_3, &g_pBrightSRV_3,
        &g_pBlurH_Texture_3, &g_pBlurH_RTV_3, &g_pBlurH_SRV_3,
        &g_pBlurV_Texture_3, &g_pBlurV_RTV_3, &g_pBlurV_SRV_3);

    // シェーダーのコンパイル
    ID3DBlob* pVSBlob = nullptr;
    ID3DBlob* pPSBrightBlob = nullptr;
    ID3DBlob* pPSDownsampleBlob = nullptr;
    ID3DBlob* pPSBlurHBlob = nullptr;
    ID3DBlob* pPSBlurVBlob = nullptr;
    ID3DBlob* pPSCompositeBlob = nullptr;

    CompileShaderFromMemory(g_VSCode, "main", "vs_5_0", &pVSBlob);
    CompileShaderFromMemory(g_PSBrightPassCode, "main", "ps_5_0", &pPSBrightBlob);
    CompileShaderFromMemory(g_PSDownsampleCode, "main", "ps_5_0", &pPSDownsampleBlob);
    CompileShaderFromMemory(g_PSBlurHCode, "main", "ps_5_0", &pPSBlurHBlob);
    CompileShaderFromMemory(g_PSBlurVCode, "main", "ps_5_0", &pPSBlurVBlob);
    CompileShaderFromMemory(g_PSCompositeCode, "main", "ps_5_0", &pPSCompositeBlob);

    if (pVSBlob) {
        pDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVS);
    }
    if (pPSBrightBlob) {
        pDevice->CreatePixelShader(pPSBrightBlob->GetBufferPointer(), pPSBrightBlob->GetBufferSize(), nullptr, &g_pPS_BrightPass);
        pPSBrightBlob->Release();
    }
    if (pPSDownsampleBlob) {
        pDevice->CreatePixelShader(pPSDownsampleBlob->GetBufferPointer(), pPSDownsampleBlob->GetBufferSize(), nullptr, &g_pPS_Downsample);
        pPSDownsampleBlob->Release();
    }
    if (pPSBlurHBlob) {
        pDevice->CreatePixelShader(pPSBlurHBlob->GetBufferPointer(), pPSBlurHBlob->GetBufferSize(), nullptr, &g_pPS_BlurH);
        pPSBlurHBlob->Release();
    }
    if (pPSBlurVBlob) {
        pDevice->CreatePixelShader(pPSBlurVBlob->GetBufferPointer(), pPSBlurVBlob->GetBufferSize(), nullptr, &g_pPS_BlurV);
        pPSBlurVBlob->Release();
    }
    if (pPSCompositeBlob) {
        pDevice->CreatePixelShader(pPSCompositeBlob->GetBufferPointer(), pPSCompositeBlob->GetBufferSize(), nullptr, &g_pPS_Composite);
        pPSCompositeBlob->Release();
    }

    // 入力レイアウト
    if (pVSBlob) {
        pDevice->CreateInputLayout(nullptr, 0, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pInputLayout);
        pVSBlob->Release();
    }

    // 定数バッファ
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.ByteWidth = sizeof(CB_Bloom);
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pDevice->CreateBuffer(&cbDesc, nullptr, &g_pCBBloom);

    // サンプラーステート
    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    pDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
}

void Bloom_Finalize()
{
    // Stage 1
    SAFE_RELEASE(g_pBrightTexture_1);
    SAFE_RELEASE(g_pBrightRTV_1);
    SAFE_RELEASE(g_pBrightSRV_1);
    SAFE_RELEASE(g_pBlurH_Texture_1);
    SAFE_RELEASE(g_pBlurH_RTV_1);
    SAFE_RELEASE(g_pBlurH_SRV_1);
    SAFE_RELEASE(g_pBlurV_Texture_1);
    SAFE_RELEASE(g_pBlurV_RTV_1);
    SAFE_RELEASE(g_pBlurV_SRV_1);

    // Stage 2
    SAFE_RELEASE(g_pBrightTexture_2);
    SAFE_RELEASE(g_pBrightRTV_2);
    SAFE_RELEASE(g_pBrightSRV_2);
    SAFE_RELEASE(g_pBlurH_Texture_2);
    SAFE_RELEASE(g_pBlurH_RTV_2);
    SAFE_RELEASE(g_pBlurH_SRV_2);
    SAFE_RELEASE(g_pBlurV_Texture_2);
    SAFE_RELEASE(g_pBlurV_RTV_2);
    SAFE_RELEASE(g_pBlurV_SRV_2);

    // Stage 3
    SAFE_RELEASE(g_pBrightTexture_3);
    SAFE_RELEASE(g_pBrightRTV_3);
    SAFE_RELEASE(g_pBrightSRV_3);
    SAFE_RELEASE(g_pBlurH_Texture_3);
    SAFE_RELEASE(g_pBlurH_RTV_3);
    SAFE_RELEASE(g_pBlurH_SRV_3);
    SAFE_RELEASE(g_pBlurV_Texture_3);
    SAFE_RELEASE(g_pBlurV_RTV_3);
    SAFE_RELEASE(g_pBlurV_SRV_3);

    SAFE_RELEASE(g_pVS);
    SAFE_RELEASE(g_pPS_BrightPass);
    SAFE_RELEASE(g_pPS_Downsample);
    SAFE_RELEASE(g_pPS_BlurH);
    SAFE_RELEASE(g_pPS_BlurV);
    SAFE_RELEASE(g_pPS_Composite);

    SAFE_RELEASE(g_pInputLayout);
    SAFE_RELEASE(g_pCBBloom);
    SAFE_RELEASE(g_pSamplerLinear);
}

void Bloom_Apply(ID3D11ShaderResourceView* pSceneTexture)
{
    // 共通設定
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->IASetInputLayout(g_pInputLayout);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    g_pContext->PSSetSamplers(0, 1, &g_pSamplerLinear);

    D3D11_VIEWPORT vp = {};
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    CB_Bloom cb;

    // ========================================
    // Stage 1: 1/2解像度の処理（通常Bloom）
    // ========================================
    vp.Width = (FLOAT)(g_Width / 2);
    vp.Height = (FLOAT)(g_Height / 2);
    g_pContext->RSSetViewports(1, &vp);

    // 1-1. 輝度抽出パス（Stage 1のパラメータを使用）
    {
        cb.threshold = g_LayerParams[0].threshold;
        cb.intensity = g_LayerParams[0].intensity;
        g_pContext->UpdateSubresource(g_pCBBloom, 0, nullptr, &cb, 0, 0);
        g_pContext->PSSetConstantBuffers(0, 1, &g_pCBBloom);

        g_pContext->OMSetRenderTargets(1, &g_pBrightRTV_1, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBrightRTV_1, clearColor);
        g_pContext->PSSetShader(g_pPS_BrightPass, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &pSceneTexture);
        g_pContext->Draw(3, 0);
    }

    // 1-2. 横ブラーパス
 {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurH_RTV_1, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurH_RTV_1, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurH, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBrightSRV_1);
        g_pContext->Draw(3, 0);
    }

    // 1-3. 縦ブラーパス
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurV_RTV_1, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurV_RTV_1, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurV, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBlurH_SRV_1);
        g_pContext->Draw(3, 0);
    }

    // ========================================
    // Stage 2: 1/4解像度の処理（ハイライト層）
    // ========================================
    vp.Width = (FLOAT)(g_Width / 4);
    vp.Height = (FLOAT)(g_Height / 4);
    g_pContext->RSSetViewports(1, &vp);

    // 2-1. 輝度抽出（Stage 2のパラメータを使用、元シーンから直接抽出）
    {
        cb.threshold = g_LayerParams[1].threshold;
        cb.intensity = g_LayerParams[1].intensity;
        g_pContext->UpdateSubresource(g_pCBBloom, 0, nullptr, &cb, 0, 0);
        g_pContext->PSSetConstantBuffers(0, 1, &g_pCBBloom);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBrightRTV_2, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBrightRTV_2, clearColor);
        g_pContext->PSSetShader(g_pPS_BrightPass, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &pSceneTexture);
        g_pContext->Draw(3, 0);
    }

    // 2-2. 横ブラーパス
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurH_RTV_2, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurH_RTV_2, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurH, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBrightSRV_2);
        g_pContext->Draw(3, 0);
    }

    // 2-3. 縦ブラーパス
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurV_RTV_2, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurV_RTV_2, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurV, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBlurH_SRV_2);
        g_pContext->Draw(3, 0);
    }

    // ========================================
    // Stage 3: 1/8解像度の処理（中間層）
    // ========================================
    vp.Width = (FLOAT)(g_Width / 8);
    vp.Height = (FLOAT)(g_Height / 8);
    g_pContext->RSSetViewports(1, &vp);

    // 3-1. 輝度抽出（Stage 3のパラメータを使用、元シーンから直接抽出）
    {
        cb.threshold = g_LayerParams[2].threshold;
        cb.intensity = g_LayerParams[2].intensity;
        g_pContext->UpdateSubresource(g_pCBBloom, 0, nullptr, &cb, 0, 0);
        g_pContext->PSSetConstantBuffers(0, 1, &g_pCBBloom);

        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBrightRTV_3, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBrightRTV_3, clearColor);
        g_pContext->PSSetShader(g_pPS_BrightPass, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &pSceneTexture);
        g_pContext->Draw(3, 0);
    }

    // 3-2. 横ブラーパス
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurH_RTV_3, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurH_RTV_3, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurH, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBrightSRV_3);
        g_pContext->Draw(3, 0);
    }

    // 3-3. 縦ブラーパス
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->OMSetRenderTargets(1, &g_pBlurV_RTV_3, nullptr);
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pContext->ClearRenderTargetView(g_pBlurV_RTV_3, clearColor);
        g_pContext->PSSetShader(g_pPS_BlurV, nullptr, 0);
        g_pContext->PSSetShaderResources(0, 1, &g_pBlurH_SRV_3);
        g_pContext->Draw(3, 0);
    }

    // ========================================
    // 最終合成パス（バックバッファに描画）
    // ========================================
    {
        ID3D11ShaderResourceView* nullSRV = nullptr;
        g_pContext->PSSetShaderResources(0, 1, &nullSRV);
        g_pContext->PSSetShaderResources(1, 1, &nullSRV);
        g_pContext->PSSetShaderResources(2, 1, &nullSRV);
        g_pContext->PSSetShaderResources(3, 1, &nullSRV);

        // バックバッファに戻す
        Direct3D_SetBackBuffer();

        // ビューポートをフル解像度に戻す
        vp.Width = (FLOAT)g_Width;
        vp.Height = (FLOAT)g_Height;
        g_pContext->RSSetViewports(1, &vp);

        // 合成用の定数バッファ（全体の強度調整）
        cb.threshold = 0.0f; // 合成時は不要
        cb.intensity = 1.0f; // 全体の強度（各層のintensityは既に適用済み）
        g_pContext->UpdateSubresource(g_pCBBloom, 0, nullptr, &cb, 0, 0);
        g_pContext->PSSetConstantBuffers(0, 1, &g_pCBBloom);

        g_pContext->PSSetShader(g_pPS_Composite, nullptr, 0);
        ID3D11ShaderResourceView* srvs[4] = { pSceneTexture, g_pBlurV_SRV_1, g_pBlurV_SRV_2, g_pBlurV_SRV_3 };
        g_pContext->PSSetShaderResources(0, 4, srvs);
        g_pContext->Draw(3, 0);

        // クリーンアップ
        ID3D11ShaderResourceView* nullSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
        g_pContext->PSSetShaderResources(0, 4, nullSRVs);
    }
}

void Bloom_SetLayerParameters(int layerIndex, float threshold, float intensity)
{
    if (layerIndex >= 0 && layerIndex < 3) {
        g_LayerParams[layerIndex].threshold = threshold;
        g_LayerParams[layerIndex].intensity = intensity;
    }
}

void Bloom_SetThreshold(float threshold)
{
    // 全層に同じ閾値を設定
    for (int i = 0; i < 3; i++) {
        g_LayerParams[i].threshold = threshold;
    }
}

void Bloom_SetIntensity(float intensity)
{
    // 全層に同じ強度を設定
    for (int i = 0; i < 3; i++) {
        g_LayerParams[i].intensity = intensity;
    }
}
