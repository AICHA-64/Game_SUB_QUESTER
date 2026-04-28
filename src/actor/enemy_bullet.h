// ----------------------------------------------------
// エネミー用弾丸の管理 [enemy_bullet.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#ifndef ENEMY_BULLET_H
#define ENEMY_BULLET_H

#include <DirectXMath.h>
#include "collision.h"

void EnemyBullet_Initialize();
void EnemyBullet_Finalize();

void EnemyBullet_Update(double elapsed_time);
void EnemyBullet_Draw();

// プレイヤーを追従する弾丸を生成
void EnemyBullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target_position, float speed);

void EnemyBullet_Destroy(int index);

int EnemyBullet_GetBulletCount();
Sphere EnemyBullet_GetSphere(int index);
const DirectX::XMFLOAT3& EnemyBullet_GetPosition(int index);

#endif // ENEMY_BULLET_H
