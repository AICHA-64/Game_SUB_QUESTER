// ----------------------------------------------------
// ゲームデータ管理 [game_data.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-04-26
// Version: 1.0
// ----------------------------------------------------
#include "game_data.h"

static double g_FinalTime = 0.0;
static bool g_DiedInFinalBoss = false;

static double g_HighScore = 0.0;
static double g_HighShot = 0.0;

void GameData_Initialize()
{
    g_FinalTime = 0.0;
    g_DiedInFinalBoss = false;
    // ハイスコア関連は初期化（リセット）しない
}

double GameData_GetFinalTime()
{
    return g_FinalTime;
}

void GameData_SetFinalTime(double time)
{
    g_FinalTime = time;
}

bool GameData_GetDiedInFinalBoss()
{
    return g_DiedInFinalBoss;
}

void GameData_SetDiedInFinalBoss(bool died)
{
    g_DiedInFinalBoss = died;
}

double GameData_GetHighScore()
{
    return g_HighScore;
}

void GameData_SetHighScore(double score)
{
    g_HighScore = score;
}

double GameData_GetHighShot()
{
    return g_HighShot;
}

void GameData_SetHighShot(double shot)
{
    g_HighShot = shot;
}
