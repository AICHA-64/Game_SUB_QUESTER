// ----------------------------------------------------
// テクスチャ描画 [texture.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-17
// Version: 1.0
// ----------------------------------------------------
#ifndef TEXTURE_H
#define TEXTURE_H

#include <d3d11.h>

void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Texture_Finalize(void);

// テクスチャ画像の読み込み
//
// 戻り値　: 管理番号。読み込めなかった場合-1。
//
int Texture_Load(const wchar_t* pFiliname);

// テクスチャの開放
void Texture_AllRelease();

void Texture_SetTexture(int texid);
void Texture_Bind(int slot, int texid); // 指定スロットにテクスチャをバインドする
void Texture_BindSRV(int slot, ID3D11ShaderResourceView* srv);
unsigned int Texture_Width(int texid);
unsigned int Texture_Height(int texid);

#endif // TEXTURE_H
