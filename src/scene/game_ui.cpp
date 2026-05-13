// ----------------------------------------------------
// ゲームシーン UI描画分割 [game_ui.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-04-18
// Version: 1.0
// ----------------------------------------------------
#include "game.h"
#include "sprite.h"
#include "direct3d.h"
#include "player.h"
#include "player_upgrade.h"
#include "shot_counter.h"
#include "boss_counter.h"
#include "timer.h"
#include "Enemy.h"
#include "enemy_final_boss.h"

// タイム表示用定数
static constexpr unsigned int TIME_CHIP_WIDTH = 32;
static constexpr unsigned int TIME_CHIP_HEIGHT = 32;
static constexpr int TIME_FONT_WIDTH = 64;
static constexpr int TIME_FONT_HEIGHT = 64;

// 強化状態表示用の定数
static constexpr float STATUS_ICON_HEIGHT = 40.0f;
static constexpr float STATUS_ICON_WIDTH = 100.0f;
static constexpr float STATUS_PADDING = 10.0f;
static constexpr float STATUS_LINE_HEIGHT = 50.0f;

void GameScene::DrawUI()
{
	Sprite_Begin();

	// HP表示
	{
		float hpBarX = 0.0f;
		float hpBarY = 0.0f;
		float hpBarWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
		float hpBarHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
		float hpRatio = (float)Player::GetInstance().GetHP() / (float)Player::GetInstance().GetMaxHP();
		
		DirectX::XMFLOAT4 hpColor;
		if (hpRatio > 0.9f) {
			hpColor = { 0.05f, 0.05f, 0.3f, 0.6f };
		} 
		else if (hpRatio > 0.5f) {
			hpColor = { 0.5f, 0.5f, 0.0f, 0.6f };
		} 
		else {
			hpColor = { 0.6f, 0.0f, 0.0f, 0.6f };
		}
		Sprite_Draw(m_UITexId, hpBarX, hpBarY, hpBarWidth, hpBarHeight, hpColor);
	}
	
	Direct3D_SetOffScreenTexture(0);
	Sprite_DrawCircle(
		-1,
		static_cast<float>(Direct3D_GetBackBufferWidth() * 0.5 - 150.0f),
		static_cast<float>(Direct3D_GetBackBufferHeight() * 0.71),
		300.0f,
		300.0f
	);

	Sprite_Draw(m_TimeTexId, 0.0f, 0.0f, 200.0f, 80.0f);
	Sprite_Draw(m_ScoreTexId, (Direct3D_GetBackBufferWidth() - 250.0f), 0.0f, 250.0f, 80.0f);
	
	float statusX = 30.0f;
	float statusY = static_cast<float>(Direct3D_GetBackBufferHeight() - 260.0f);
	DrawUpgradeStatus(statusX, statusY);
	
	Sprite_Draw(m_TargetTexId, (Direct3D_GetBackBufferWidth() - 300.0f), (Direct3D_GetBackBufferHeight() - 85.0f), 300.0f, 85.0f);

	Sprite_Draw(
		m_MapTexId,
		static_cast<float>(Direct3D_GetBackBufferWidth() * 0.5f - 150.0f),
		static_cast<float>(Direct3D_GetBackBufferHeight() * 0.71f),
		300.0f,
		300.0f
	);

	// ミニマップに合わせたレーダーの走査線
	{
		float centerX = static_cast<float>(Direct3D_GetBackBufferWidth() * 0.5f);
		float centerY = static_cast<float>(Direct3D_GetBackBufferHeight() * 0.71f) + 150.0f;
		float radius = 150.0f;
		
		// レーダーの線の終点を計算
		float endX = centerX + sinf(m_RadarAngle) * radius;
		float endY = centerY - cosf(m_RadarAngle) * radius;
		
		DirectX::XMFLOAT4 radarColor = { 0.0f, 1.0f, 0.0f, 0.5f }; // ここで色
		Sprite_DrawLine(
			m_WhiteTexId,
			{ centerX, centerY },
			{ endX, endY },
			radarColor
		);
		
		// 残像っぽいの
		for (int i = 1; i <= 20; ++i) {
			float angleOffset = m_RadarAngle - (0.02f * i);
			float trailX = centerX + sinf(angleOffset) * radius;
			float trailY = centerY - cosf(angleOffset) * radius;
			
			float alpha = 0.5f - (0.025f * i);
			if (alpha < 0.0f) alpha = 0.0f;
			
			DirectX::XMFLOAT4 trailColor = { 0.0f, 1.0f, 0.0f, alpha };
			Sprite_DrawLine(
				m_WhiteTexId,
				{ centerX, centerY },
				{ trailX, trailY },
				trailColor
			);
		}
	}

	Sprite_Draw(
		m_reticleTexId,
		static_cast<float>(Direct3D_GetBackBufferWidth() * 0.5f - 75.0f),
		static_cast<float>(Direct3D_GetBackBufferHeight() * 0.5f - 75.0f),
		150.0f,
		150.0f
	);

	ShotCounter_Draw(1700.0f, 80.0f);
	BossCounter_Draw(1780.0f, 920.0f);
	Timer_Draw();
	
	// ラスボス戦モードの場合、レベルを画面中央上に表示
	if (Enemy_IsFinalBossMode()) {
		int finalBossLevel = PlayerUpgrade_GetFinalBossLevel();
		
		// レベル表示位置を計算
		float screenCenterX = static_cast<float>(Direct3D_GetBackBufferWidth()) * 0.5f;
		float levelY = 50.0f; // 上部に表示
		
		// レベル数字の幅を計算して中央揃え
		float numberWidth = (finalBossLevel >= 10) ? (TIME_FONT_WIDTH * 2.0f) : TIME_FONT_WIDTH;
		float levelX = screenCenterX - (numberWidth * 0.5f);
		
		// レベル数字を表示
		if (finalBossLevel >= 10) {
			DrawNumber(levelX, levelY, finalBossLevel / 10);
			DrawNumber(levelX + TIME_FONT_WIDTH, levelY, finalBossLevel % 10);
		} else {
			DrawNumber(levelX, levelY, finalBossLevel);
		}
		
		// ラスボスのHPバーを表示
		// ラスボスを探してHPを取得
		int bossCurrentHP = 0;
		int bossMaxHP = 1;
		for (int i = 0; i < Enemy_GetEnemyCount(); i++) {
			if (Enemy_GetEnemy(i)->IsFinalBoss()) {
				// dynamic_castでEnemyFinalBossにキャスト
				auto* finalBoss = dynamic_cast<class EnemyFinalBoss*>(Enemy_GetEnemy(i));
				if (finalBoss) {
					bossCurrentHP = finalBoss->GetHp();
					bossMaxHP = finalBoss->GetMaxHp();
				}
				break;
			}
		}
		
		// HPバーの描画
		float hpBarWidth = 600.0f;  // HPバーの幅
		float hpBarHeight = 20.0f;  // HPバーの高さ
		float hpBarX = screenCenterX - (hpBarWidth * 0.5f);  // 中央揃え
		float hpBarY = levelY + TIME_FONT_HEIGHT + 10.0f;  // レベル表示の下
		
		// HPの割合を計算
		float hpRatio = static_cast<float>(bossCurrentHP) / static_cast<float>(bossMaxHP);
		if (hpRatio < 0.0f) hpRatio = 0.0f;
		if (hpRatio > 1.0f) hpRatio = 1.0f;
		
		// HPバー背景（黒）
		DirectX::XMFLOAT4 bgColor = { 0.2f, 0.2f, 0.2f, 0.8f };
		Sprite_Draw(m_WhiteTexId, hpBarX, hpBarY, hpBarWidth, hpBarHeight, bgColor);
		
		// HPバー本体（色はHP残量で変化）
		DirectX::XMFLOAT4 hpColor;
		if (hpRatio > 0.6f) {
			hpColor = { 0.2f, 1.0f, 0.2f, 1.0f }; // 緑（HP多い）
		} else if (hpRatio > 0.3f) {
			hpColor = { 1.0f, 1.0f, 0.0f, 1.0f }; // 黄色（HP中）
		} else {
			hpColor = { 1.0f, 0.2f, 0.2f, 1.0f }; // 赤（HP少ない）
		}
		
		float currentHpBarWidth = hpBarWidth * hpRatio;
		Sprite_Draw(m_WhiteTexId, hpBarX, hpBarY, currentHpBarWidth, hpBarHeight, hpColor);
		
		// HPバー枠（白い縁）
		float borderThickness = 2.0f;
		DirectX::XMFLOAT4 borderColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		// 上の枠
		Sprite_Draw(m_WhiteTexId, hpBarX, hpBarY - borderThickness, hpBarWidth, borderThickness, borderColor);
		// 下の枠
		Sprite_Draw(m_WhiteTexId, hpBarX, hpBarY + hpBarHeight, hpBarWidth, borderThickness, borderColor);
		// 左の枠
		Sprite_Draw(m_WhiteTexId, hpBarX - borderThickness, hpBarY, borderThickness, hpBarHeight, borderColor);
		// 右の枠
		Sprite_Draw(m_WhiteTexId, hpBarX + hpBarWidth, hpBarY, borderThickness, hpBarHeight, borderColor);
	}
}

// 強化タイプに対応するテクスチャIDを取得
int GameScene::GetUpgradeTextureId(UpgradeType type)
{
	if (type >= 0 && type < UPGRADE_TYPE_COUNT) {
		return m_UpgradeTexIds[type];
	}
	return -1;
}

void GameScene::DrawNumber(float x, float y, int number)
{
	if (number < 0 || number > 9) return;
	Sprite_Draw(m_TimeChipTexId, x, y, TIME_FONT_WIDTH, TIME_FONT_HEIGHT, 
		TIME_CHIP_WIDTH * number, 0, TIME_CHIP_WIDTH, TIME_CHIP_HEIGHT);
}

void GameScene::DrawUpgradeStatus(float x, float y)
{
	float currentY = y;
	
	for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
		UpgradeType type = static_cast<UpgradeType>(i);
		int level = PlayerUpgrade_GetLevel(type);
		
		// アイコンを描画
		int iconTexId = GetUpgradeTextureId(type);
		
		if (iconTexId != -1) {
			DirectX::XMFLOAT4 iconColor = {1.0f, 1.0f, 1.0f, 1.0f};
			
			// レベルが0の場合は暗く表示
			if (level == 0) {
				iconColor = {0.3f, 0.3f, 0.3f, 0.5f};
			}
			
			Sprite_Draw(iconTexId, x, currentY, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, iconColor);
		}
		
		// レベルを表示
		float numberX = x + STATUS_ICON_WIDTH + STATUS_PADDING;
		float numberY = currentY + (STATUS_ICON_HEIGHT - TIME_FONT_HEIGHT) * 0.5f;
		
		if (level >= 10) {
			DrawNumber(numberX, numberY, level / 10);
			DrawNumber(numberX + TIME_FONT_WIDTH, numberY, level % 10);
		} else {
			DrawNumber(numberX, numberY, level);
		}
		
		currentY += STATUS_LINE_HEIGHT;
	}
}
