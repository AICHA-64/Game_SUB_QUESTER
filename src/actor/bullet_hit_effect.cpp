// ----------------------------------------------------
// 弾の衝突effectの表示 [bullet_hit_effect.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-19
// Version: 1.0
// ----------------------------------------------------
#include "bullet_hit_effect.h"
#include "texture.h"
using namespace DirectX;
#include "sprite_anim.h"
#include "billboard.h"
#include "Enemy.h"

static int g_TexId = -1;
static int g_AnimId = -1;


class BulletHitEffect
{
private:
		XMFLOAT3 m_position{};
		int m_anim_play_id{ -1 };
		bool m_is_destroy{ false };
		
public:
	BulletHitEffect(const XMFLOAT3& position)
		: m_position(position), m_anim_play_id(SpriteAnim_CreatePlayer(g_AnimId)){

	}
	~BulletHitEffect() {
		SpriteAnim_DestroyPlayer(m_anim_play_id);
	}

	void Update();
	void Draw() const;

	bool IsDestroy() const {
		return m_is_destroy;
	}
};

static constexpr int EFFECT_MAX = 256;
static BulletHitEffect* g_pEffects[EFFECT_MAX]{};
static int g_EffectCount{ 0 };

void BulletHitEffect_Initialize()
{
	g_TexId = Texture_Load(L"resource/texture/Explosion.png");

	g_AnimId = SpriteAnim_RegisterPattern(
		g_TexId, 16, 4, 0.1f, { 256,256 }, { 0,0 }, false);

	g_EffectCount = 0;
}

void BulletHitEffect_Finalize()
{
	for (int i = 0; i < g_EffectCount; i++) {
		delete g_pEffects[i];
	}
}

void BulletHitEffect_Update()
{
	for (int i = 0; i < g_EffectCount; i++) {
		g_pEffects[i]->Update();
	}

	for (int i = g_EffectCount - 1; i >= 0; i--) {
		if (g_pEffects[i]->IsDestroy()) {
			delete g_pEffects[i];
			g_pEffects[i] = g_pEffects[g_EffectCount - 1];
			g_EffectCount--;
		}
	}
}

void BulletHitEffect_Draw()
{
	for (int i = 0; i < g_EffectCount; i++) {
		g_pEffects[i]->Draw();
	}
}

void BulletHitEffect_Create(const DirectX::XMFLOAT3& position)
{
	g_pEffects[g_EffectCount] = new BulletHitEffect(position);
	g_EffectCount++;
}

void BulletHitEffect::Update()
{
	if (SpriteAnim_IsStopped(m_anim_play_id)) {
		m_is_destroy = true;
	}
}

void BulletHitEffect::Draw() const
{
	BillboardAnim_Draw(m_anim_play_id, m_position, { 1.0f, 1.0f });
}
