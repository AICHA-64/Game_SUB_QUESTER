// ----------------------------------------------------
// プレイヤー強化システム [player_upgrade.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-12
// Version: 1.0
// ----------------------------------------------------
#include "player_upgrade.h"
#include <cstdlib>
#include <ctime>

// 各強化タイプのレベル（0から開始）
static int g_UpgradeLevels[UPGRADE_TYPE_COUNT] = {};

// 現在のラウンド数（0から開始、初回は0）
static int g_CurrentRound = 0;

// ラスボスのレベル（0から開始、撃破ごとに+1）
static int g_FinalBossLevel = 0;

// 乱数初期化フラグ
static bool g_RandomInitialized = false;

void PlayerUpgrade_Initialize()
{
    // 乱数の初期化（一度だけ）
    if (!g_RandomInitialized) {
        srand(static_cast<unsigned int>(time(nullptr)));
        g_RandomInitialized = true;
    }
    
    // 全強化レベルをリセット
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        g_UpgradeLevels[i] = 0;
    }
    
    g_CurrentRound = 0; // 初回は0から開始
    g_FinalBossLevel = 0; // ラスボスレベルもリセット
}

void PlayerUpgrade_Apply(UpgradeType type)
{
    if (type >= 0 && type < UPGRADE_TYPE_COUNT) {
      g_UpgradeLevels[type]++;
    }
}

int PlayerUpgrade_GetLevel(UpgradeType type)
{
    if (type >= 0 && type < UPGRADE_TYPE_COUNT) {
        return g_UpgradeLevels[type];
    }
    return 0;
}

float PlayerUpgrade_GetMoveSpeedMultiplier()
{
    // 1レベルごとに15%アップ
    return 1.0f + g_UpgradeLevels[UPGRADE_MOVE_SPEED] * 0.15f;
}

float PlayerUpgrade_GetShotIntervalMultiplier()
{
 // 1レベルごとに20%短縮（0.8倍、0.64倍...）
    float multiplier = 1.0f;
    for (int i = 0; i < g_UpgradeLevels[UPGRADE_SHOT_INTERVAL]; i++) {
        multiplier *= 0.8f;
    }
    return multiplier;
}

int PlayerUpgrade_GetMaxHPBonus()
{
    // 1レベルごとに+1 HP
    return g_UpgradeLevels[UPGRADE_MAX_HP];
}

float PlayerUpgrade_GetBulletSpeedMultiplier()
{
    // 1レベルごとに20%アップ
    return 1.0f + g_UpgradeLevels[UPGRADE_BULLET_SPEED] * 0.2f;
}

float PlayerUpgrade_GetDamageMultiplier()
{
 // 1レベルごとに25%アップ
    return 1.0f + g_UpgradeLevels[UPGRADE_DAMAGE] * 0.25f;
}

void PlayerUpgrade_GetRandomChoices(UpgradeType* outChoice1, UpgradeType* outChoice2)
{
    // ランダムに2つの異なる強化タイプを選択
    int choice1 = rand() % UPGRADE_TYPE_COUNT;
    int choice2 = rand() % UPGRADE_TYPE_COUNT;
    
    // 同じものが選ばれた場合は別のものを
    while (choice2 == choice1) {
        choice2 = rand() % UPGRADE_TYPE_COUNT;
    }
    
    *outChoice1 = static_cast<UpgradeType>(choice1);
    *outChoice2 = static_cast<UpgradeType>(choice2);
}

const char* PlayerUpgrade_GetTypeName(UpgradeType type)
{
    switch (type) {
    case UPGRADE_MOVE_SPEED:    return "Speed Up";
    case UPGRADE_SHOT_INTERVAL: return "Rapid Fire";
    case UPGRADE_MAX_HP:        return "HP Up";
    case UPGRADE_BULLET_SPEED:  return "Bullet Speed";
    case UPGRADE_DAMAGE:        return "Power Up";
    default:          return "Unknown";
    }
}

const char* PlayerUpgrade_GetTypeDescription(UpgradeType type)
{
    switch (type) {
    case UPGRADE_MOVE_SPEED: return "Movement speed +15%";
    case UPGRADE_SHOT_INTERVAL: return "Shot interval -20%";
  case UPGRADE_MAX_HP:        return "Max HP +1";
    case UPGRADE_BULLET_SPEED:  return "Bullet speed +20%";
    case UPGRADE_DAMAGE:        return "Damage +25%";
    default: return "";
    }
}

int PlayerUpgrade_GetRound()
{
    return g_CurrentRound;
}

void PlayerUpgrade_IncrementRound()
{
    g_CurrentRound++;
}

void PlayerUpgrade_Reset()
{
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        g_UpgradeLevels[i] = 0;
    }
    g_CurrentRound = 0; // 完全リセットは0に戻る
    g_FinalBossLevel = 0; // ラスボスレベルもリセット
}

int PlayerUpgrade_GetFinalBossLevel()
{
    return g_FinalBossLevel;
}

void PlayerUpgrade_IncrementFinalBossLevel()
{
    g_FinalBossLevel++;
}

void PlayerUpgrade_ResetFinalBossLevel()
{
    g_FinalBossLevel = 0;
}
