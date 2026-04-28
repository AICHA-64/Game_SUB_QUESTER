// ----------------------------------------------------
// タイマー [timer.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-07
// Version: 1.0
// ----------------------------------------------------
#ifndef TIMER_H
#define TIMER_H

void Timer_Initialize(float x, float y);
void Timer_Finalize();
void Timer_Update(double elapsed_time);
void Timer_Draw();

void Timer_Reset();
double Timer_GetTime();

#endif // TIMER_H
