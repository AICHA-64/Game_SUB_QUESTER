// ----------------------------------------------------
// 射撃数カウンター [shot_counter.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-08
// Version: 1.0
// ----------------------------------------------------
#ifndef SHOT_COUNTER_H
#define SHOT_COUNTER_H

void ShotCounter_Initialize(float x, float y, int digit);
void ShotCounter_Finalize();
void ShotCounter_Add();
unsigned int ShotCounter_GetScore();
void ShotCounter_Reset();

void ShotCounter_Draw(float x, float y);
void ShotCounter_Draw2(float x, float y, int shotCount);

#endif // SHOT_COUNTER_H
