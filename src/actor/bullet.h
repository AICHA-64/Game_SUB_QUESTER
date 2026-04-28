// ----------------------------------------------------
// 弾丸の管理 [bullet.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-12
// Version: 1.0
// ----------------------------------------------------
#ifndef BULLET_H
#define BULLET_H

#include <DirectXMath.h>
#include "collision.h"

void Bullet_Initialize();
void Bullet_Finalize();

void Bullet_Update(double elapsed_time);
void Bullet_Draw();

// シャドウマップ専用描画関数（深度のみ）
void Bullet_DrawShadow();

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity);
void Bullet_Destroy(int index);

int Bullet_GetBulletCount();
AABB Bullet_GetAABB(int index);
Sphere Bullet_GetSphere(int index);
const DirectX::XMFLOAT3& Bullet_GetPosition(int index);
const DirectX::XMFLOAT3& Bullet_GetVelocity(int index);

// 弾丸がインターセプト対象として既に割り当てられているかを追跡
bool Bullet_IsInterceptAssigned(int index);
void Bullet_SetInterceptAssigned(int index, bool assigned);

#endif // BULLET_H
