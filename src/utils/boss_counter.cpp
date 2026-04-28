// ----------------------------------------------------
// ボスカウンター表示 [boss_counter.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-15
// Version: 1.0
// ----------------------------------------------------
#include "boss_counter.h"
#include "sprite.h"
#include "texture.h"
#include <algorithm>

static int g_BossCounterTexId = -1;
static int g_BossCount = 0;
static int g_MaxDigits = 2;
static float g_InitX = 0.0f;
static float g_InitY = 0.0f;

static constexpr unsigned int BOSS_COUNTER_Chip_WIDTH = 32;
static constexpr unsigned int BOSS_COUNTER_Chip_HEIGHT = 32;
static constexpr int BOSS_COUNTER_FONT_WIDTH = 64;
static constexpr int BOSS_COUNTER_FONT_HEIGHT = 64;

static void drawNumberBossCounter(float x, float y, int number);

void BossCounter_Initialize(float x, float y, int maxDigits)
{
	g_InitX = x;
	g_InitY = y;
	g_MaxDigits = maxDigits;
	g_BossCount = 0;
	
	// 数字テクスチャの読み込み
	g_BossCounterTexId = Texture_Load(L"resource/texture/chip1.png");
}

void BossCounter_Finalize()
{
	g_BossCounterTexId = -1;
}

void BossCounter_Draw(float x, float y)
{
	if (g_BossCounterTexId == -1) return;

	unsigned int temp = g_BossCount;
	
	// 最大桁数で制限
	unsigned int maxValue = 1;
	for (int i = 0; i < g_MaxDigits; i++) {
		maxValue *= 10;
	}
	maxValue--;
	temp = std::min(temp, maxValue);

	// 各桁を描画
	for (int i = 0; i < g_MaxDigits; i++)
	{
		int n = temp % 10;
		float digitX = x + (g_MaxDigits - 1 - i) * BOSS_COUNTER_FONT_WIDTH;
		drawNumberBossCounter(digitX, y, n);
		temp /= 10;
	}
}

void BossCounter_SetCount(int count)
{
	g_BossCount = count;
}

int BossCounter_GetCount()
{
	return g_BossCount;
}

static void drawNumberBossCounter(float x, float y, int number)
{
	if (number < 0 || number >= 10) {
		return;
	}
	Sprite_Draw(g_BossCounterTexId, x, y, (float)BOSS_COUNTER_FONT_WIDTH, (float)BOSS_COUNTER_FONT_HEIGHT, 
		BOSS_COUNTER_Chip_WIDTH * number, 0, BOSS_COUNTER_Chip_WIDTH, BOSS_COUNTER_Chip_HEIGHT);
}
