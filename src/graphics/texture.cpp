// ----------------------------------------------------
// テクスチャ描画 [texture.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-06-18
// Version: 1.0
// ----------------------------------------------------

#include "texture.h"
#include "direct3d.h"
#include <string>
#include "WICTextureLoader11.h"
using namespace DirectX;


static constexpr int TEXTURE_MAX = 1024; // テクスチャ管理最大数


struct Texture
{
	std::wstring filename;
	unsigned int width;
	unsigned int height;
	ID3D11Resource* pTexture = nullptr;
	ID3D11ShaderResourceView* pTextureView = nullptr;
};


static Texture g_Textures[TEXTURE_MAX]{};
static int g_SetTextureIndex = -1;

// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;


void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	for (Texture& t : g_Textures){
		t.pTexture = nullptr;
	}

	g_SetTextureIndex = -1;

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;
}

void Texture_Finalize(void)
{
	Texture_AllRelease();
}

int Texture_Load(const wchar_t* pFiliname)
{
	// すでに読み込んだファイルは読み込まない
	for (int i = 0; i < TEXTURE_MAX; i++) {
		if (g_Textures[i].filename == pFiliname) {
			return i;
		}
	}

	// 空いている管理領域を探す
	for (int i = 0; i < TEXTURE_MAX; i++)
	{
		if (g_Textures[i].pTexture) continue; // 使用中

		// テクスチャの読み込み
		HRESULT hr;

		
		hr = CreateWICTextureFromFile(g_pDevice, g_pContext, pFiliname, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);
		
		ID3D11Texture2D* pTexture = (ID3D11Texture2D*)g_Textures[i].pTexture;
		D3D11_TEXTURE2D_DESC t2desc;
		pTexture->GetDesc(&t2desc);
		g_Textures[i].width = t2desc.Width;
		g_Textures[i].height = t2desc.Height;

		if (FAILED(hr)) {
			MessageBoxW(nullptr, L"テクスチャの初期化に失敗しました", pFiliname, MB_OK | MB_ICONERROR);
			return -1;
		}

		g_Textures[i].filename = pFiliname;

		return i;
	}
	
	return -1; // 管理領域が満杯になったら返す
}

void Texture_AllRelease()
{
	for (Texture& t : g_Textures) {
		t.filename.clear();
		SAFE_RELEASE(t.pTexture);
		SAFE_RELEASE(t.pTextureView);
	}
}

void Texture_SetTexture(int texid)
{
	if (texid < 0) return;

	g_SetTextureIndex = texid;

	// テクスチャ設定
	g_pContext->PSSetShaderResources(0, 1, & g_Textures[texid].pTextureView);
}

void Texture_Bind(int slot, int texid)
{
	if (slot < 0) return;

	ID3D11ShaderResourceView* srv = nullptr;
	if (texid >= 0 && texid < TEXTURE_MAX) {
		srv = g_Textures[texid].pTextureView;
	}
	// slot に 1 個だけバインド
	g_pContext->PSSetShaderResources(slot, 1, &srv);
}

void Texture_BindSRV(int slot, ID3D11ShaderResourceView* srv)
{
	if (slot < 0) return;
	// そのまま 1 枚バインド
	g_pContext->PSSetShaderResources(slot, 1, &srv);
}

unsigned int Texture_Width(int texid)
{
	if (texid < 0) return 0;

	return g_Textures[texid].width;
}

unsigned int Texture_Height(int texid)
{
	if (texid < 0) return 0;

	return g_Textures[texid].height;
}
