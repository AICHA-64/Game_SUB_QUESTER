// ----------------------------------------------------
// イージングキューブ [easing_cube.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-1-23
// Version: 1.0
// ----------------------------------------------------
#include "easing_cube.h"
using namespace DirectX;
#include "cube.h"
#include <algorithm>

void EasingCube::Update(double elapsed_time)
{
	if (!m_IsStart)
	{
		return;
	}
	m_AccumlatedTime += elapsed_time;

	if (m_AccumlatedTime > m_MoveTime)
	{
		m_AccumlatedTime = m_MoveTime;
	}
}

void EasingCube::Draw() const
{
	XMVECTOR start = XMLoadFloat3(&m_StartPosition);
	XMVECTOR end = XMLoadFloat3(&m_EndPosition);
	XMVECTOR v = end - start; // スタートからエンドへのベクトル

	double ratio = static_cast<float>(std::min(m_AccumlatedTime / m_MoveTime, 1.0));
	v *= static_cast<float>(ratio); // 線形補間
	v += start; // スタート位置を足す

	XMMATRIX world = XMMatrixTranslationFromVector(v);
	Cube_Draw(2, world);
}
