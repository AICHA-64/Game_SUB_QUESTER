// ----------------------------------------------------
// タイマー [timer.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-07
// Version: 1.0
// ----------------------------------------------------

#include "timer.h"
#include "texture.h"
#include "sprite.h"

static double g_ElapsedTime = 0.0;
static float g_x = 0.0f, g_y = 0.0f;
static int g_TimeTexId = -1;

// score.cpp と同じ定数を使用
static constexpr unsigned int SCORE_Chip_WIDTH = 32;
static constexpr unsigned int SCORE_Chip_HEIGHT = 32;
static constexpr int SCORE_FONT_WIDTH = 64;
static constexpr int SCORE_FONT_HEIGHT = 64;
static constexpr int SCORE_FONT_WIDTH_SHORT = 32;

static void drawNumber(float x, float y, int number);
static void drawColon(float x, float y);

void Timer_Initialize(float x, float y)
{
	g_ElapsedTime = 0.0;
	g_x = x;
	g_y = y;
	g_TimeTexId = Texture_Load(L"resource/texture/chip1.png");
}

void Timer_Finalize()
{

}

void Timer_Update(double elapsed_time)
{
	g_ElapsedTime += elapsed_time;
}

void Timer_Draw()
{
	int minutes = (int)(g_ElapsedTime / 60);
	int seconds = (int)g_ElapsedTime % 60;
	int milliseconds = (int)((g_ElapsedTime - (int)g_ElapsedTime) * 100);

	float current_x = g_x;

	// 分 (2桁)
	drawNumber(current_x, g_y, minutes / 10);
	current_x += SCORE_FONT_WIDTH;
	drawNumber(current_x, g_y, minutes % 10);
	current_x += SCORE_FONT_WIDTH;

	drawColon(current_x, g_y);
	current_x += SCORE_FONT_WIDTH_SHORT;

	// 秒 (2桁)
	drawNumber(current_x, g_y, seconds / 10);
	current_x += SCORE_FONT_WIDTH;
	drawNumber(current_x, g_y, seconds % 10);
	current_x += SCORE_FONT_WIDTH;

	drawColon(current_x, g_y);
	current_x += SCORE_FONT_WIDTH_SHORT;

	// ミリ秒 (2桁)
	drawNumber(current_x, g_y, milliseconds / 10);
	current_x += SCORE_FONT_WIDTH;
	drawNumber(current_x, g_y, milliseconds % 10);
}

void Timer_Reset()
{
	g_ElapsedTime = 0.0;
}

double Timer_GetTime()
{
	return g_ElapsedTime;
}


void drawNumber(float x, float y, int number)
{
	if (number < 0 || number > 9) return;
	Sprite_Draw(g_TimeTexId, x, y, SCORE_FONT_WIDTH, SCORE_FONT_HEIGHT, 32 * number, 0, SCORE_Chip_WIDTH, SCORE_Chip_HEIGHT);
}

void drawColon(float x, float y)
{
	Sprite_Draw(g_TimeTexId, x, y, SCORE_FONT_WIDTH_SHORT, SCORE_FONT_HEIGHT, 32 * 10, 0, SCORE_Chip_WIDTH, SCORE_Chip_HEIGHT);
}
