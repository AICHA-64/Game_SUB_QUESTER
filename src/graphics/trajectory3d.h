// ----------------------------------------------------
// 軌跡制御3D [trajectory3d.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-11-21
// Version: 1.0
// ----------------------------------------------------
#ifndef TRAJECTORY3D_H
#define TRAJECTORY3D_H

#include <DirectXMath.h>

void Trajectory3D_Initialize();
void Trajectory3D_Finalize();
void Trajectory3D_Update(double elapsed_time);
void Trajectory3D_Draw();

void Trajectory3D_Create(const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT4& color,
	float size,
	double life_time
);

#endif // #TRAJECTORY3D_H
