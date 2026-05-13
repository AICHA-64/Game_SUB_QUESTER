// ----------------------------------------------------
// マップ表示用カメラ [map_camera.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-08
// Version: 1.0
// ----------------------------------------------------
#include "map_camera.h"
using namespace DirectX;

static XMFLOAT3 g_Position{};
static XMFLOAT3 g_Front{ 0.0f, 1.0f, 1.0f };

static float g_OrthoHalfWidth = 80.0f;
static float g_OrthoHalfHeight = 80.0f;

void MapCamera_Initialize()
{
}

void MapCamera_Finalize()
{
}

void MapCamera_SetPosition(const DirectX::XMFLOAT3& position)
{
	g_Position = position;
}

void MapCamera_SetFront(const DirectX::XMFLOAT3& front)
{
	g_Front = front;
}

void MapCamera_SetOrthoSize(float halfWidth, float halfHeight)
{
	g_OrthoHalfWidth = halfWidth;
	g_OrthoHalfHeight = halfHeight;
}

void MapCamera_SetOrthoScale(float scale)
{
	if (scale <= 0.0f) return;
	g_OrthoHalfWidth *= scale;
	g_OrthoHalfHeight *= scale;
}

const DirectX::XMFLOAT4X4& MapCamera_GetViewMatrix()
{
	static XMFLOAT4X4 mtxView{};
	XMMATRIX view = XMMatrixLookToLH(
		XMLoadFloat3(&g_Position),
		XMVECTOR{ 0.0f, -1.0f, 0.0f },
		XMLoadFloat3(&g_Front)
	);

	XMStoreFloat4x4(&mtxView, view);
	return mtxView;
}

const DirectX::XMFLOAT4X4& MapCamera_GetProjectionMatrix()
{
	static XMFLOAT4X4 mtxProj{};
	// 可変の範囲を使用
	XMMATRIX proj = XMMatrixOrthographicOffCenterLH(
		- g_OrthoHalfWidth,  g_OrthoHalfWidth,
		- g_OrthoHalfHeight, g_OrthoHalfHeight,
		0.1f, 1000.0f
	);

	XMStoreFloat4x4(&mtxProj, proj);
	return mtxProj;
}
