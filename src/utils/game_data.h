// ----------------------------------------------------
// ゲームデータ管理 [game_data.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-04-26
// Version: 1.0
// ----------------------------------------------------
#ifndef GAME_DATA_H
#define GAME_DATA_H

// 初期化
void GameData_Initialize();

// 最終タイム（現在のプレイのデータ）
double GameData_GetFinalTime();
void GameData_SetFinalTime(double time);

// ラスボス戦での死亡フラグ（現在のプレイのデータ）
bool GameData_GetDiedInFinalBoss();
void GameData_SetDiedInFinalBoss(bool died);

// ハイスコアデータ
double GameData_GetHighScore();
void GameData_SetHighScore(double score);

// ハイショットデータ
double GameData_GetHighShot();
void GameData_SetHighShot(double shot);

#endif // GAME_DATA_H
