// ----------------------------------------------------
// 軌跡制御 [trajectory.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-03
// Version: 1.0
// ----------------------------------------------------

#include "trajectory.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "texture.h"
#include "sprite.h"
#include "direct3d.h"



struct Trajectory
{
	XMFLOAT2 position;
	XMFLOAT4 color;
	float size;
	double lifeTime; // 経過時間
	double birthTime;  // 最大時間 0.0なら生まれていない
};

static constexpr unsigned int TRAJECTORY_MAX{ 1024 };
static Trajectory g_Trajectorys[TRAJECTORY_MAX]{};
static int g_TrajectoryTexId = -1;
static double g_Time = 0.0; // 全体の経過時間



void Trajectory_Initialize()
{
	for (Trajectory& t : g_Trajectorys) {
		t.birthTime = 0.0;
	}

	g_Time = 0.0;

	g_TrajectoryTexId = Texture_Load(L"resource/texture/effect000.jpg");

}

void Trajectory_Finalize()
{
}

void Trajectory_Update(double elapsed_time)
{
	for (Trajectory& t : g_Trajectorys) {

		if (t.birthTime == 0.0) continue;

		double time = g_Time - t.birthTime;

		if (time > t.lifeTime) {
			t.birthTime = 0.0; // 寿命尽き
		}
	}
	g_Time += elapsed_time; // 時間更新
}

void Trajectory_Draw()
{
	Direct3D_SetAlphaBlendAdd();

	for (const Trajectory& t : g_Trajectorys) {

		if (t.birthTime == 0.0) continue;

		double time = g_Time - t.birthTime;
		float ratio = (float)(time / t.lifeTime);
		float size = t.size * (1.0f - ratio); // 徐々に小さくなる

		XMFLOAT4 color = t.color;
		color.w = t.color.w * (1.0f - ratio); // 徐々に透明になる

		Sprite_Draw
		(
			g_TrajectoryTexId,
			t.position.x + 25.0f,
			t.position.y + 20.0f,
			size,
			size,
			t.color // 徐々に透明になる
		);
	}
	Direct3D_SetAlphaBlendTransparent();
}

void Trajectory_Create(const DirectX::XMFLOAT2& position, const DirectX::XMFLOAT4& color, float size, double life_time)
{
	for (Trajectory& t : g_Trajectorys) {
		if (t.birthTime != 0.0) continue;

		t.birthTime = g_Time;
		t.lifeTime = life_time;
		t.color = color;
		t.position = position;
		t.size = size;

		break;
	}
}
