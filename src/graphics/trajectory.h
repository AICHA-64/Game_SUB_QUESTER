// ----------------------------------------------------
// 軌跡制御 [trajectory.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-09-03
// Version: 1.0
// ----------------------------------------------------
#ifndef TRAJECTORY_H
#define TRAJECTORY_H

#include <DirectXMath.h>

void Trajectory_Initialize();
void Trajectory_Finalize();
void Trajectory_Update(double elapsed_time);
void Trajectory_Draw();

void Trajectory_Create(const DirectX::XMFLOAT2& position, 
					   const DirectX::XMFLOAT4& color,
					   float size,
					   double life_time
					   );

#endif // #TRAJECTORY_H
