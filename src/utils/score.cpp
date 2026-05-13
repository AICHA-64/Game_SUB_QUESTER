// ----------------------------------------------------
// スコア管理 [score.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-09
// Version: 1.0
// ----------------------------------------------------

#include "score.h"
#include "texture.h"
#include "sprite.h"
#include <algorithm>

static unsigned int g_Score = 0; // スコア
static unsigned int g_ViewScore = 0;

static int g_Digit = 1; // 桁数
static unsigned int g_CounterStop = 1; // カンスト
static float g_x = 0.0f, g_y = 0.0f; // スコアの座標
static int g_ScoreTexId = -1; // スコアのテクスチャID
static constexpr unsigned int SCORE_Chip_WIDTH = 32; // 素材の切り取りサイズ
static constexpr unsigned int SCORE_Chip_HEIGHT = 32; // 素材の切り取りサイズ
static constexpr int SCORE_FONT_WIDTH = 64; // スコアの幅
static constexpr int SCORE_FONT_HEIGHT = 64; // スコアの幅


static void drawNumber(float x, float y, int number);


void Score_Initialize(float x, float y, int digit)
{
	g_Score = 0;
	g_ViewScore = 0;
	g_Digit = digit;
	g_x = x;
	g_y = y;

	// カンストの設定
	for (int i = 0; i < digit; i++)
	{
		g_CounterStop *= 10;
	}

	g_CounterStop--; // カンストは9999...のように1引く
	
	g_ScoreTexId = Texture_Load(L"resource/texture/chip1.png");

}

void Score_Finalize()
{
}

void Score_Update()
{
	g_ViewScore = std::min(g_ViewScore + 1, g_Score); // スコアを徐々に表示する
}

void Score_Draw()
{
	unsigned int temp = std::min(g_ViewScore , g_CounterStop);

	for (int i = 0; i < g_Digit; i++)
	{
		int n = temp % 10; // 1の位を取得

		float x = g_x + (g_Digit - 1 - i) * SCORE_FONT_WIDTH; // 描画位置を計算
		drawNumber(x, g_y, n); // 数字を描画

		temp /= 10; // 次の位へ
	}
}

unsigned int Score_GetScore()
{
	return g_Score;
}

void Score_AddScore(int score)
{
	g_ViewScore = g_Score;
	g_Score += score;
}

void Score_Reset()
{
	g_Score = 0;
}

void drawNumber(float x, float y, int number)
{
	if (number < 0 || number >= 10) {
		return; // 数字が範囲外の場合は何もしない
	}
	//Sprite_Draw(g_ScoreTexId, x, y, SCORE_FONT_WIDTH * number, 0, SCORE_FONT_WIDTH, SCORE_FONT_HEIGHT);
	Sprite_Draw(g_ScoreTexId, x, y, SCORE_FONT_WIDTH, SCORE_FONT_HEIGHT, SCORE_Chip_WIDTH * number, 0, SCORE_Chip_WIDTH, SCORE_Chip_HEIGHT);
}
