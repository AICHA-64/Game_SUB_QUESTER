// ----------------------------------------------------
// Bloomエフェクト [bloom.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-15
// Version: 1.0
// ----------------------------------------------------
#ifndef BLOOM_H
#define BLOOM_H

#include <d3d11.h>

void Bloom_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, int width, int height);
void Bloom_Finalize();

// メインシーンにBloom効果を適用する
// pSceneTexture: メインシーンが描画されたテクスチャ(SRV)
// Bloom適用後の結果をバックバッファに描画
void Bloom_Apply(ID3D11ShaderResourceView* pSceneTexture);

// 各ステージの閾値と強度を個別に設定（ハイライト層と通常層を分けて調整可能）
// layerIndex: 0=Stage1(1/2解像度), 1=Stage2(1/4解像度), 2=Stage3(1/8解像度)
void Bloom_SetLayerParameters(int layerIndex, float threshold, float intensity);

void Bloom_SetThreshold(float threshold);
void Bloom_SetIntensity(float intensity);

#endif // BLOOM_H
