// ----------------------------------------------------
// ボスカウンター表示 [boss_counter.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-15
// Version: 1.0
// ----------------------------------------------------
#ifndef BOSS_COUNTER_H
#define BOSS_COUNTER_H

void BossCounter_Initialize(float x, float y, int maxDigits);
void BossCounter_Finalize();
void BossCounter_Draw(float x, float y);
void BossCounter_SetCount(int count);
int BossCounter_GetCount();

#endif // BOSS_COUNTER_H
