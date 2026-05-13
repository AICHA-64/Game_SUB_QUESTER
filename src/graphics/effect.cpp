// ----------------------------------------------------
// エフェクトの描画 [effect.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-04
// Version: 1.0
// ----------------------------------------------------

#include "effect.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "texture.h"
#include "sprite_anim.h"
#include "Audio.h"


struct Effect
{
	XMFLOAT2 position;
	// XMFLOAT2 velocity;
	// double lifeTime;
	int sprite_anim_id; // スプライトアニメーションID
	bool isEnable; // trueなら有効
};

static constexpr int EFFECT_MAX{ 256 };
static Effect g_Effects[EFFECT_MAX]{};
static int g_EffectTexId = -1;
static int g_AnimPatternId = -1;
static int g_EffectSoundId = -1;

void Effect_Initialize()
{
	for (Effect& e : g_Effects) {
		e.isEnable = false;
	}

	g_EffectTexId = Texture_Load(L"resource/texture/explosion.png");
	g_EffectSoundId = LoadAudio("resource/audio/explosion.wav");
	g_AnimPatternId = SpriteAnim_RegisterPattern
		(
			g_EffectTexId,
			16, // パターン数
			4, // 横のパターン数
			0.01, // 1パターンの時間
			{ 256, 256 }, // パターンサイズ
			{ 0, 0 }, // 開始位置
			false
			);
}

void Effect_Finalize()
{
	UnloadAudio(g_EffectSoundId);
}

void Effect_Update(double)
{
	for (Effect& e : g_Effects) {

		if (!e.isEnable) continue; // 有効じゃないとここから下に到達しない
		
		if (SpriteAnim_IsStopped(e.sprite_anim_id)) {
			SpriteAnim_DestroyPlayer(e.sprite_anim_id); // アニメーションが終わったら削除
			e.isEnable = false; // エフェクトを無効にする
		}
	}
}

void Effect_Draw()
{


	for (Effect& e : g_Effects) {

		if (!e.isEnable) continue; // 有効じゃないとここから下に到達しない
		// ワールド座標から画面座標へ変換
		float screen_x = e.position.x;
		float screen_y = e.position.y ;

		SpriteAnim_Draw(e.sprite_anim_id, screen_x, screen_y, 64.0f, 64.0f);
	}

}

void Effect_Create(const DirectX::XMFLOAT2& position)
{
	for (Effect& e : g_Effects) {
		if (e.isEnable) continue; // 空いてる場所を探す
		e.isEnable = true;
		e.position = position;
		e.sprite_anim_id = SpriteAnim_CreatePlayer(g_AnimPatternId);
		PlayAudio(g_EffectSoundId);
		break;
	}
}
