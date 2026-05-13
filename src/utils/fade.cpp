// ----------------------------------------------------
// フェードイン・アウト制御 [fade.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-10
// Version: 1.0
// ----------------------------------------------------
#include "fade.h"
using namespace DirectX;
#include <algorithm>
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"

static double g_FadeTime	   { 0.0 }; // フェード時間
static double g_FadeStartTime  { 0.0 }; // フェードカウンター
static double g_AccumulatedTime{ 0.0 }; // 経過時間
static XMFLOAT3 g_Color		   { 0.0f, 0.0f, 0.0f }; // フェード色　黒
static float g_Alpha		   { 0.0f }; // アルファ値（透明度）
static FadeState g_State	   { FADE_STATE_NONE }; // フェード状態

static int g_FadeTexId = -1; // フェード用のテクスチャID

void Fade_Initialize()
{
	g_FadeTime = 0.0;
	g_FadeStartTime = 0.0;
	g_AccumulatedTime = 0.0;
	g_Color = { 0.0f, 0.0f, 0.0f }; // 初期は黒
	g_Alpha = 0.0f;
	g_State = FADE_STATE_NONE; // 初期状態はなし

	g_FadeTexId = Texture_Load(L"resource/texture/white.png"); // フェード用のテクスチャを読み込む
}

void Fade_Finalize()
{
}

void Fade_Update(double elapsed_time)
{
	if (g_State != FADE_STATE_IN && g_State != FADE_STATE_OUT) return;

	g_AccumulatedTime += elapsed_time;

	double ratio = std::min((g_AccumulatedTime - g_FadeStartTime) / g_FadeTime, 1.0);

	if (ratio >= 1.0) {
		g_State = g_State == FADE_STATE_IN ? FADE_STATE_FINISHED_IN : FADE_STATE_FINISHED_OUT;
	}

	g_Alpha = (float)(g_State == FADE_STATE_IN ? 1.0 - ratio : ratio);
}

void Fade_Draw()
{
	//if (g_State <= FADE_STATE_NONE) return; // 透明なポリゴンなので描かない
	//if (g_State <= FADE_STATE_FINISHED_IN) return;

	if (g_State == FADE_STATE_NONE) return; // 透明なポリゴンなので描かない
	if (g_State == FADE_STATE_FINISHED_IN) return;

	XMFLOAT4 color{ g_Color.x, g_Color.y, g_Color.z, g_Alpha };
	Sprite_Draw(g_FadeTexId, 0.0f, 0.0f, (float)Direct3D_GetBackBufferWidth(), (float)Direct3D_GetBackBufferHeight(),color);
}

void Fade_Start(double time, bool isFadeOut, DirectX::XMFLOAT3 color)
{
	g_FadeStartTime = g_AccumulatedTime;
	g_FadeTime = time;
	g_State = isFadeOut ? FADE_STATE_OUT : FADE_STATE_IN;
	g_Color = color;
	g_Alpha = isFadeOut ? 0.0f : 1.0f;
}

FadeState Fade_GetState()
{
	return g_State;
}
