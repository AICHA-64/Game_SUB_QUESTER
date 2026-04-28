// ----------------------------------------------------
// 射撃数カウンター [shot_counter.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-08
// Version: 1.0
// ----------------------------------------------------

#include "shot_counter.h"
#include "texture.h"
#include "sprite.h"
#include <algorithm>

static unsigned int g_ShotCount = 0;

static int g_Digit = 1;
static unsigned int g_CounterStop = 1;
static float g_x = 0.0f, g_y = 0.0f;
static int g_ShotCounterTexId = -1;
static constexpr unsigned int SHOT_COUNTER_Chip_WIDTH = 32;
static constexpr unsigned int SHOT_COUNTER_Chip_HEIGHT = 32;
static constexpr int SHOT_COUNTER_FONT_WIDTH = 64;
static constexpr int SHOT_COUNTER_FONT_HEIGHT = 64;

static void drawNumberShotCounter(float x, float y, int number);

void ShotCounter_Initialize(float x, float y, int digit)
{
    g_ShotCount = 0;
    g_Digit = digit;
    g_x = x;
    g_y = y;

    g_CounterStop = 1;
    for (int i = 0; i < digit; i++)
    {
        g_CounterStop *= 10;
    }
    g_CounterStop--;

    g_ShotCounterTexId = Texture_Load(L"resource/texture/chip1.png");
}

void ShotCounter_Finalize()
{
}

void ShotCounter_Add()
{
    g_ShotCount++;
}

unsigned int ShotCounter_GetScore()
{
    return g_ShotCount;
}

void ShotCounter_Reset()
{
    g_ShotCount = 0;
}

void ShotCounter_Draw(float x, float y)
{
    unsigned int temp = std::min(g_ShotCount, g_CounterStop);

    for (int i = 0; i < g_Digit; i++)
    {
        int n = temp % 10;

        float local_x = x + (g_Digit - 1 - i) * SHOT_COUNTER_FONT_WIDTH;
        drawNumberShotCounter(local_x, y, n);

        temp /= 10;
    }

}

void ShotCounter_Draw2(float x, float y, int shotCount)
{
    g_ShotCount = shotCount;

    unsigned int temp = std::min(g_ShotCount, g_CounterStop);

    for (int i = 0; i < g_Digit; i++)
    {
        int n = temp % 10;

        float local_x = x + (g_Digit - 1 - i) * SHOT_COUNTER_FONT_WIDTH;
        drawNumberShotCounter(local_x, y, n);

        temp /= 10;
    }
}

static void drawNumberShotCounter(float x, float y, int number)
{
    if (number < 0 || number >= 10) {
        return;
    }
    Sprite_Draw(g_ShotCounterTexId, x, y, (float)SHOT_COUNTER_FONT_WIDTH, (float)SHOT_COUNTER_FONT_HEIGHT, SHOT_COUNTER_Chip_WIDTH * number, 0, SHOT_COUNTER_Chip_WIDTH, SHOT_COUNTER_Chip_HEIGHT);
}
