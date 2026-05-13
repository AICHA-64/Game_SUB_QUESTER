// ----------------------------------------------------
// プレイヤー強化システム [player_upgrade.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-12
// Version: 1.0
// ----------------------------------------------------
#ifndef PLAYER_UPGRADE_H
#define PLAYER_UPGRADE_H

// 強化タイプ
enum UpgradeType
{
    UPGRADE_MOVE_SPEED,      // 移動速度アップ
    UPGRADE_SHOT_INTERVAL,   // 弾発射間隔短縮
    UPGRADE_MAX_HP,          // 最大HPアップ
    UPGRADE_BULLET_SPEED,    // 弾速アップ
    UPGRADE_DAMAGE,    // ダメージアップ
    UPGRADE_TYPE_COUNT
};

void PlayerUpgrade_Initialize();

// 強化を適用
void PlayerUpgrade_Apply(UpgradeType type);

// 現在の強化レベルを取得
int PlayerUpgrade_GetLevel(UpgradeType type);

// 強化による倍率を取得
float PlayerUpgrade_GetMoveSpeedMultiplier();
float PlayerUpgrade_GetShotIntervalMultiplier();
int PlayerUpgrade_GetMaxHPBonus();
float PlayerUpgrade_GetBulletSpeedMultiplier();
float PlayerUpgrade_GetDamageMultiplier();

// ランダムな2つの強化タイプを取得（選択肢用）
void PlayerUpgrade_GetRandomChoices(UpgradeType* outChoice1, UpgradeType* outChoice2);

// 強化タイプの名前を取得
const char* PlayerUpgrade_GetTypeName(UpgradeType type);

// 強化タイプの説明を取得
const char* PlayerUpgrade_GetTypeDescription(UpgradeType type);

// 現在のラウンド数を取得・設定
int PlayerUpgrade_GetRound();
void PlayerUpgrade_IncrementRound();

// 全ての強化をリセット（タイトルに戻る時など）
void PlayerUpgrade_Reset();

// ラスボスレベルの管理
int PlayerUpgrade_GetFinalBossLevel();
void PlayerUpgrade_IncrementFinalBossLevel();
void PlayerUpgrade_ResetFinalBossLevel();

#endif // PLAYER_UPGRADE_H
