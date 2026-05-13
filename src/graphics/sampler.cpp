// ----------------------------------------------------
// サンプラーの設定ユーティリティー [sampler.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-09
// Version: 1.0
// ----------------------------------------------------

#include "sampler.h"
#include "direct3d.h"

static ID3D11SamplerState* g_pSamplerFilterPoint = nullptr;
static ID3D11SamplerState* g_pSamplerFilterLinear = nullptr;
static ID3D11SamplerState* g_pSamplerFilterAnisotropic = nullptr;
static ID3D11SamplerState* g_pShadowSampler = nullptr;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


void Sampler_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// サンプラーステート設定
	D3D11_SAMPLER_DESC sampler_desc{};

	// フィルタリング
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

	// UV参照外の取り扱い（UVアドレッシングモード）
	sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampler_desc.MipLODBias = 0;
	sampler_desc.MaxAnisotropy = 16;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sampler_desc.MinLOD = 0;
	sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;

	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterPoint);

	// フィルタリング
	sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterLinear);

	// フィルタリング
	sampler_desc.Filter = D3D11_FILTER_ANISOTROPIC;
	g_pDevice->CreateSamplerState(&sampler_desc, &g_pSamplerFilterAnisotropic);

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	sampDesc.BorderColor[0] = 1.0f;
	sampDesc.BorderColor[1] = 1.0f;
	sampDesc.BorderColor[2] = 1.0f;
	sampDesc.BorderColor[3] = 1.0f;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	pDevice->CreateSamplerState(&sampDesc, &g_pShadowSampler);
}

void Sampler_Finalize(void)
{
	SAFE_RELEASE(g_pSamplerFilterAnisotropic);
	SAFE_RELEASE(g_pSamplerFilterLinear);
	SAFE_RELEASE(g_pSamplerFilterPoint);
	SAFE_RELEASE(g_pShadowSampler);
}

void Sampler_SetFilterPoint()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterPoint);
}

void Sampler_SetFilterLinear()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterLinear);
}

void Sampler_SetFilterAnisotropic()
{
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerFilterAnisotropic);
}

void Sampler_SetShadowSampler(ID3D11DeviceContext* pContext)
{
    pContext->PSSetSamplers(1, 1, &g_pShadowSampler);
}
