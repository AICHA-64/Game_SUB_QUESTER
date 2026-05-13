// ----------------------------------------------------
// 弾の衝突effectの表示 [bullet_hit_effect.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-19
// Version: 1.0
// ----------------------------------------------------
#ifndef BULLET_HIT_EFFECT_H
#define BULLET_HIT_EFFECT_H

#include <DirectXMath.h>

void BulletHitEffect_Initialize();
void BulletHitEffect_Finalize();
void BulletHitEffect_Update();
void BulletHitEffect_Draw();

void BulletHitEffect_Create(const DirectX::XMFLOAT3& position);


#endif // BULLET_HIT_EFFECT_H
