// ----------------------------------------------------
// 軌跡制御3D [trajectory3d.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-11-21
// Version: 1.0
// ----------------------------------------------------

#include "trajectory3d.h"
using namespace DirectX;
#include "texture.h"
#include "billboard.h"
#include "direct3d.h"
#include "shader_billboard.h"
#include <cstdlib>
#include <cmath>



struct Trajectory3D
{
	XMFLOAT3 position;
	XMFLOAT3 velocity;      // 泡の移動速度（上昇＋揺れ）
	XMFLOAT4 color;
	float size;
	float baseSize;         // 元のサイズ
	double lifeTime;        // 経過時間
	double birthTime;       // 最大時間 0.0なら生まれていない
	float wobblePhase;      // 揺れの位相
	float wobbleSpeed;      // 揺れの速度
};

static constexpr unsigned int TRAJECTORY_MAX{ 1024 };
static Trajectory3D g_Trajectorys[TRAJECTORY_MAX]{};
static int g_TrajectoryTexId = -1;
static double g_Time = 0.0; // 全体の経過時間

// ランダムな浮動小数点を生成 (min ~ max)
static float RandomFloat(float min, float max)
{
	return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void Trajectory3D_Initialize()
{
	for (Trajectory3D& t : g_Trajectorys) {
		t.birthTime = 0.0;
	}

	g_Time = 0.0;

	// 泡のテクスチャを読み込み
	g_TrajectoryTexId = Texture_Load(L"resource/texture/bubble2.png");

}

void Trajectory3D_Finalize()
{
}

void Trajectory3D_Update(double elapsed_time)
{
	float dt = static_cast<float>(elapsed_time);

	for (Trajectory3D& t : g_Trajectorys) {

		if (t.birthTime == 0.0) continue;

		double time = g_Time - t.birthTime;

		if (time > t.lifeTime) {
			t.birthTime = 0.0; // 寿命尽き
			continue;
		}

		// 泡の動き：ゆっくり上昇＋横に揺れる
		float wobble = sinf(t.wobblePhase) * 0.3f;
		t.position.x += (t.velocity.x + wobble) * dt;
		t.position.y += t.velocity.y * dt;
		t.position.z += t.velocity.z * dt;

		// 揺れの位相を更新
		t.wobblePhase += t.wobbleSpeed * dt;
	}
	g_Time += elapsed_time; // 時間更新
}

void Trajectory3D_Draw()
{
	Direct3D_SetAlphaBlendAdd();

	for (const Trajectory3D& t : g_Trajectorys) {

		if (t.birthTime == 0.0) continue;

		double time = g_Time - t.birthTime;
		float ratio = (float)(time / t.lifeTime);
		
		// 泡は最初小さく、途中で最大になり、最後に消える
		float sizeRatio;
		if (ratio < 0.2f) {
			// 最初の20%で膨らむ
			sizeRatio = ratio / 0.2f;
		} else if (ratio < 0.7f) {
			// 20%～70%は最大サイズ維持
			sizeRatio = 1.0f;
		} else {
			// 70%以降で縮む
			sizeRatio = 1.0f - ((ratio - 0.7f) / 0.3f);
		}
		float size = t.baseSize * sizeRatio;

		XMFLOAT4 color = t.color;
		// 徐々に透明になる（後半で急速に）
		if (ratio > 0.6f) {
			color.w = t.color.w * (1.0f - ((ratio - 0.6f) / 0.4f));
		}

		Billboard_Draw(g_TrajectoryTexId, t.position, {size, size}, color);
	}

	Direct3D_SetAlphaBlendTransparent();
}

void Trajectory3D_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4& color, float size, double life_time)
{
	for (Trajectory3D& t : g_Trajectorys) {
		if (t.birthTime != 0.0) continue;

		t.birthTime = g_Time;
		t.lifeTime = life_time;
		
		// 泡らしい色（水色～白）にオーバーライド
		t.color = { 0.6f, 0.9f, 1.0f, 0.7f };
		(void)color;
		
		// 少しランダムな位置にずらす
		t.position.x = position.x + RandomFloat(-0.1f, 0.1f);
		t.position.y = position.y + RandomFloat(-0.1f, 0.1f);
		t.position.z = position.z + RandomFloat(-0.1f, 0.1f);
		
		// 泡の上昇速度（ゆっくり上に＋少し横に）
		t.velocity.x = RandomFloat(-0.2f, 0.2f);
		t.velocity.y = RandomFloat(0.3f, 0.8f);  // 上昇
		t.velocity.z = RandomFloat(-0.2f, 0.2f);
		
		// サイズにランダム性を追加
		t.baseSize = size * RandomFloat(0.5f, 1.2f);
		t.size = t.baseSize;
		
		// 揺れのパラメータ
		t.wobblePhase = RandomFloat(0.0f, 6.28f);  // 0～2π
		t.wobbleSpeed = RandomFloat(3.0f, 6.0f);

		break;
	}
}
