// ----------------------------------------------------
// スカイドーム [sky.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-21
// Version: 1.0
// ----------------------------------------------------
#include "sky.h"
using namespace DirectX;
#include "model.h"
#include "shader3d_unlit.h"

static MODEL* g_pModelSky = nullptr;
static XMFLOAT3 g_SkyPosition = { 0.0f, 0.0f, 0.0f };


void Sky_Initialize()
{
	g_pModelSky = ModelLoad("resource/model/sky.fbx", 100.0f, true);
}

void Sky_Finalize()
{
	ModelRelease(g_pModelSky);
}

void Sky_SetPosition(const DirectX::XMFLOAT3& position)
{
	// X, Y, Z全ての座標をプレイヤー（カメラ）に追従させる
	g_SkyPosition = position;
}

void Sky_Draw()
{
	Shader3d_Unlit_Begin();

	ModelDrawUnlit(g_pModelSky, XMMatrixTranslationFromVector(XMLoadFloat3(&g_SkyPosition)));
}
