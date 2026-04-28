// ----------------------------------------------------
// 水面通過エフェクト [surface_transition_effect.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-01-20
// Version: 1.0
// ----------------------------------------------------
#ifndef SURFACE_TRANSITION_EFFECT_H
#define SURFACE_TRANSITION_EFFECT_H

#include <DirectXMath.h>

// 水面通過エフェクトの初期化
void SurfaceTransitionEffect_Initialize();

// 水面通過エフェクトの終了処理
void SurfaceTransitionEffect_Finalize();

// 水面通過エフェクトの更新
void SurfaceTransitionEffect_Update(double elapsed_time);

// 水面通過エフェクトの描画
void SurfaceTransitionEffect_Draw();

// 浮上エフェクトをトリガー（水中→水上）
void SurfaceTransitionEffect_TriggerSurface(const DirectX::XMFLOAT3& position);

// 潜航エフェクトをトリガー（水上→水中）
void SurfaceTransitionEffect_TriggerDive(
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& frontDirection);

// 水中での定期的な泡生成の更新（プレイヤーが水中にいるかどうかと前方向を渡す）
// playerVelocity: プレイヤーの速度ベクトル
void SurfaceTransitionEffect_UpdateUnderwaterBubbles(
	double elapsed_time, 
	bool isUnderwater, 
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& frontDirection,
	const DirectX::XMFLOAT3& velocity);

#endif // SURFACE_TRANSITION_EFFECT_H
