// ----------------------------------------------------
// 水中エフェクト [underwater_effect.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-01-10
// Version: 1.0
// ----------------------------------------------------
#ifndef UNDERWATER_EFFECT_H
#define UNDERWATER_EFFECT_H

void UnderwaterEffect_Initialize();
void UnderwaterEffect_Finalize();
void UnderwaterEffect_Update(float playerY);
void UnderwaterEffect_Draw();

// 水面の高度を取得
float UnderwaterEffect_GetWaterSurfaceY();

#endif // UNDERWATER_EFFECT_H
