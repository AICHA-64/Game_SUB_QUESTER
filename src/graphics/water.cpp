// ----------------------------------------------------
// ??????? [water.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-22
// Version: 1.2
// ----------------------------------------------------

#include "water.h"
#include "water_reflect.h"
#include "direct3d.h"
#include "texture.h"
#include "sampler.h"
#include "shader3d.h"
using namespace DirectX;

#include <vector>
#include <d3dcompiler.h>

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static ID3D11Buffer* g_pVertexBuffer = nullptr;
static ID3D11Buffer* g_pIndexBuffer = nullptr;

static ID3D11VertexShader* g_pVS = nullptr;
static ID3D11PixelShader* g_pPS = nullptr;
static ID3D11InputLayout* g_pInputLayout = nullptr;

static ID3D11Buffer* g_pVSConstantBuffer = nullptr;
static ID3D11Buffer* g_pPSConstantBuffer = nullptr;
static ID3D11Buffer* g_pVSWorldBuffer = nullptr;
static ID3D11Buffer* g_pVSReflectionMatrixCB = nullptr;
static ID3D11Buffer* g_pVSLightMatrixCB = nullptr;
static XMFLOAT4X4    g_ReflectionViewProj{};
static XMFLOAT4X4    g_LightViewProj{};

static ID3D11RasterizerState* g_pRasterizerStateNoCull = nullptr;

static int g_DiffuseTexId = -1;
static int g_NormalTexId = -1;

static double g_Time = 0.0;
static DirectX::XMFLOAT3 g_CameraPos = { 0,0,0 };

extern WaterRT g_WaterRT;

struct VERT
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
    XMFLOAT2 uv;
};

struct VS_CB
{
    float time;
    float waveAmplitude;
    float waveFrequency;
    float waveSpeed;
    float baseHeight;
    float pad0;
    float pad1;
    float pad2;
};
static_assert(sizeof(VS_CB) == 32, "VS_CB size must be 32 bytes");

struct PS_CB
{
    XMFLOAT4 uvScroll01;
    XMFLOAT4 waterColor;
    XMFLOAT4 cameraPos_fresnel;
};

static constexpr int   Water_W = 32;
static constexpr int   Water_H = 32;
static constexpr float WATER_SIZE = 320.0f;
static constexpr float WATER_BASEHEIGHT = 0.3f;
static constexpr float WAVE_AMPLITUDE = 0.25f;
static constexpr float WAVE_FREQ = 0.25f;
static constexpr float WAVE_SPEED = 0.7f;
static constexpr float FRESNEL_POWER = 5.0f;

static bool g_bWaterInitialized = false;

void Water_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    g_pDevice = pDevice;
    g_pContext = pContext;

    g_DiffuseTexId = Texture_Load(L"resource/texture/noise.png");
    g_NormalTexId = Texture_Load(L"resource/texture/normalmap.png");

    std::vector<VERT> vertices;
    vertices.reserve((Water_W + 1) * (Water_H + 1));
    for (int z = 0; z <= Water_H; ++z)
    {
        for (int x = 0; x <= Water_W; ++x)
        {
            float fx = ((float)x / Water_W - 0.5f) * WATER_SIZE;
            float fz = ((float)z / Water_H - 0.5f) * WATER_SIZE;

            VERT v{};
            v.pos = { fx, 0.0f, fz };
            v.normal = { 0.0f, 1.0f, 0.0f };
            v.uv = { (float)x / Water_W * 4.0f, (float)z / Water_H * 4.0f };
            vertices.emplace_back(v);
        }
    }

    std::vector<unsigned int> indices;
    indices.reserve(Water_W * Water_H * 6);
    for (int z = 0; z < Water_H; ++z)
    {
        for (int x = 0; x < Water_W; ++x)
        {
            int i0 = x + (Water_W + 1) * z;
            int i1 = x + 1 + (Water_W + 1) * z;
            int i2 = x + (Water_W + 1) * (z + 1);
            int i3 = x + 1 + (Water_W + 1) * (z + 1);

            indices.push_back(i0); indices.push_back(i2); indices.push_back(i1);
            indices.push_back(i1); indices.push_back(i2); indices.push_back(i3);
        }
    }

    D3D11_BUFFER_DESC bd{};
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = (UINT)(sizeof(VERT) * vertices.size());
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = vertices.data();
    if (FAILED(g_pDevice->CreateBuffer(&bd, &srd, &g_pVertexBuffer))) return;

    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = (UINT)(sizeof(unsigned int) * indices.size());
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    srd.pSysMem = indices.data();
    if (FAILED(g_pDevice->CreateBuffer(&bd, &srd, &g_pIndexBuffer))) return;

    // ?V?F?[?_??O?? HLSL ?t?@?C??????R???p?C??????
    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errBlob = nullptr;

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    // ???_?V?F?[?_
    hr = D3DCompileFromFile(L"shaders/shader_vertex_water.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "vs_5_0", compileFlags, 0, &vsBlob, &errBlob);
    if (FAILED(hr))
    {
#ifdef _DEBUG
        if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); errBlob->Release(); }
#else
        if (errBlob) { errBlob->Release(); }
#endif
        if (vsBlob) vsBlob->Release();
        return;
    }
    if (FAILED(g_pDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVS)))
    {
        if (vsBlob) vsBlob->Release();
        return;
    }

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (FAILED(g_pDevice->CreateInputLayout(layout, _countof(layout), vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout)))
    {
        vsBlob->Release();
        return;
    }
    vsBlob->Release();

    // ?s?N?Z???V?F?[?_
    hr = D3DCompileFromFile(L"shaders/shader_pixel_water.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main", "ps_5_0", compileFlags, 0, &psBlob, &errBlob);
    if (FAILED(hr))
    {
#ifdef _DEBUG
        if (errBlob) { OutputDebugStringA((char*)errBlob->GetBufferPointer()); errBlob->Release(); }
#else
        if (errBlob) { errBlob->Release(); }
#endif
        if (psBlob) psBlob->Release();
        return;
    }
    if (FAILED(g_pDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPS)))
    {
        if (psBlob) psBlob->Release();
        return;
    }
    psBlob->Release();

    D3D11_BUFFER_DESC cbd{};
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(VS_CB);
    g_pDevice->CreateBuffer(&cbd, nullptr, &g_pVSConstantBuffer);

    cbd.ByteWidth = sizeof(PS_CB);
    g_pDevice->CreateBuffer(&cbd, nullptr, &g_pPSConstantBuffer);

    XMFLOAT4X4 worldFloat4x4;
    XMStoreFloat4x4(&worldFloat4x4, XMMatrixIdentity());
    D3D11_BUFFER_DESC wbd{};
    wbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    wbd.ByteWidth = sizeof(XMFLOAT4X4);
    wbd.Usage = D3D11_USAGE_DEFAULT;
    D3D11_SUBRESOURCE_DATA wsr{};
    wsr.pSysMem = &worldFloat4x4;
    g_pDevice->CreateBuffer(&wbd, &wsr, &g_pVSWorldBuffer);

    D3D11_BUFFER_DESC rbd{};
    rbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    rbd.ByteWidth = sizeof(XMFLOAT4X4);
    rbd.Usage = D3D11_USAGE_DEFAULT;
    XMFLOAT4X4 reflInit;
    XMStoreFloat4x4(&reflInit, XMMatrixIdentity());
    D3D11_SUBRESOURCE_DATA rsr{};
    rsr.pSysMem = &reflInit;
    g_pDevice->CreateBuffer(&rbd, &rsr, &g_pVSReflectionMatrixCB);

    D3D11_BUFFER_DESC lbd{};
    lbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lbd.ByteWidth = sizeof(XMFLOAT4X4);
    lbd.Usage = D3D11_USAGE_DEFAULT;
    XMFLOAT4X4 lightInit;
    XMStoreFloat4x4(&lightInit, XMMatrixIdentity());
    D3D11_SUBRESOURCE_DATA lsr{};
    lsr.pSysMem = &lightInit;
    g_pDevice->CreateBuffer(&lbd, &lsr, &g_pVSLightMatrixCB);

    XMStoreFloat4x4(&g_ReflectionViewProj, XMMatrixIdentity());
    XMStoreFloat4x4(&g_LightViewProj, XMMatrixIdentity());

    D3D11_RASTERIZER_DESC rdNoCull = {};
    rdNoCull.FillMode = D3D11_FILL_SOLID;
    rdNoCull.CullMode = D3D11_CULL_NONE;
    rdNoCull.DepthClipEnable = TRUE;
    rdNoCull.MultisampleEnable = FALSE;
    g_pDevice->CreateRasterizerState(&rdNoCull, &g_pRasterizerStateNoCull);

    g_bWaterInitialized = true;
}

void Water_Finalize()
{
    if (g_pRasterizerStateNoCull) g_pRasterizerStateNoCull->Release();
    if (g_pVSLightMatrixCB) g_pVSLightMatrixCB->Release();
    if (g_pVSReflectionMatrixCB) g_pVSReflectionMatrixCB->Release();
    if (g_pVSWorldBuffer) g_pVSWorldBuffer->Release();
    if (g_pPSConstantBuffer) g_pPSConstantBuffer->Release();
    if (g_pVSConstantBuffer) g_pVSConstantBuffer->Release();
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pPS) g_pPS->Release();
    if (g_pVS) g_pVS->Release();
    if (g_pIndexBuffer) g_pIndexBuffer->Release();
    if (g_pVertexBuffer) g_pVertexBuffer->Release();
}

void Water_Update(double elapsed_time)
{
    g_Time += elapsed_time;
}

void Water_Draw()
{
	// ?????`??
    if (!g_bWaterInitialized) return;
    if (!g_pVertexBuffer) return;

	// ???o?b?t?@??g??p?????[?^?????Z?b?g
    VS_CB vs{};
    vs.time = (float)g_Time;
    vs.waveAmplitude = WAVE_AMPLITUDE;
    vs.waveFrequency = WAVE_FREQ;
    vs.waveSpeed = WAVE_SPEED;
    vs.baseHeight = WATER_BASEHEIGHT;
    g_pContext->UpdateSubresource(g_pVSConstantBuffer, 0, nullptr, &vs, 0, 0);

	// ?s?N?Z???V?F?[?_????o?b?t?@??UV?X?N???[??????F?A?J??????u????Z?b?g
    PS_CB ps{};
    XMFLOAT2 scroll0((float)fmod(g_Time * 0.02, 1.0), 0.0f);
    XMFLOAT2 scroll1(0.0f, (float)fmod(g_Time * 0.015, 1.0));
    ps.uvScroll01 = XMFLOAT4(scroll0.x, scroll0.y, scroll1.x, scroll1.y);
    ps.waterColor = XMFLOAT4(0.55f, 0.85f, 1.0f, 1.0f);
    ps.cameraPos_fresnel = XMFLOAT4(g_CameraPos.x, g_CameraPos.y, g_CameraPos.z, FRESNEL_POWER);
    g_pContext->UpdateSubresource(g_pPSConstantBuffer, 0, nullptr, &ps, 0, 0);

	// ???_?V?F?[?_????o?b?t?@????[???h?s?????s??A???C?g?s???Z?b?g
    XMFLOAT4X4 refMtxT;
    XMStoreFloat4x4(&refMtxT, XMMatrixTranspose(XMLoadFloat4x4(&g_ReflectionViewProj)));
    g_pContext->UpdateSubresource(g_pVSReflectionMatrixCB, 0, nullptr, &refMtxT, 0, 0);

	// ???C?g?s?????l??]?u????Z?b?g
    XMFLOAT4X4 lightMtxT;
    XMStoreFloat4x4(&lightMtxT, XMMatrixTranspose(XMLoadFloat4x4(&g_LightViewProj)));
    g_pContext->UpdateSubresource(g_pVSLightMatrixCB, 0, nullptr, &lightMtxT, 0, 0);

	// ???[???h?s???P??s?????
    UINT stride = sizeof(VERT);
    UINT offset = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
    g_pContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ?V?F?[?_???????C?A?E?g??Z?b?g
    g_pContext->VSSetShader(g_pVS, nullptr, 0);
    g_pContext->PSSetShader(g_pPS, nullptr, 0);
    g_pContext->IASetInputLayout(g_pInputLayout);

	// ???o?b?t?@??Z?b?g
    g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer);
    g_pContext->VSSetConstantBuffers(0, 1, &g_pVSWorldBuffer);
    g_pContext->VSSetConstantBuffers(4, 1, &g_pVSReflectionMatrixCB);
    g_pContext->VSSetConstantBuffers(5, 1, &g_pVSLightMatrixCB);

    g_pContext->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer);

	// ?e?N?X?`????T???v???[??Z?b?g
    Texture_SetTexture(g_DiffuseTexId);
    Texture_Bind(1, g_NormalTexId);
    Texture_BindSRV(2, g_WaterRT.pReflectionSRV);
    Direct3D_SetShadowMapTexture(3);
    Sampler_SetShadowSampler(g_pContext);
    Sampler_SetFilterAnisotropic();

    ID3D11RasterizerState* prevRasterizerState = nullptr;
    g_pContext->RSGetState(&prevRasterizerState);
    if (g_pRasterizerStateNoCull) {
        g_pContext->RSSetState(g_pRasterizerStateNoCull);
    }

    Direct3D_SetDepthEnable(true);
    Direct3D_SetAlphaBlendTransparent();
    Direct3D_SetDepthWriteDisable();

	// ????????`??????A?J?????O?????????`??
    g_pContext->DrawIndexed(Water_W * Water_H * 6, 0, 0);

    if (prevRasterizerState) {
        g_pContext->RSSetState(prevRasterizerState);
        prevRasterizerState->Release();
    }

    Direct3D_SetDepthWriteDisable();
}

void Water_SetCameraPosition(const DirectX::XMFLOAT3& cameraPos)
{
    g_CameraPos = cameraPos;
}

void Water_SetReflectionMatrix(const DirectX::XMFLOAT4X4& viewProjReflected)
{
    g_ReflectionViewProj = viewProjReflected;
}

void Water_SetLightMatrix(const DirectX::XMFLOAT4X4& lightViewProj)
{
    g_LightViewProj = lightViewProj;
}
