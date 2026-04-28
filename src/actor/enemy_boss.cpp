// ----------------------------------------------------
// ボス敵制御 [enemy_boss.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#include "enemy_boss.h"
#include "collision.h"
#include "player.h"
#include "enemy_bullet.h"
#include "debug_draw.h"
#include "Audio.h"
#include <cmath>
using namespace DirectX;

static int g_EnebullSeId = -1;

MODEL* EnemyBoss::s_pModel = nullptr;

void EnemyBoss_InitializeResource()
{
	if (EnemyBoss::s_pModel == nullptr) {
		EnemyBoss::s_pModel = ModelLoad("resource/model/boss.fbx", 2.5f, false);
	}

	g_EnebullSeId = LoadAudio("resource/audio/enebul.wav");
}

void EnemyBoss_FinalizeResource()
{
	if (EnemyBoss::s_pModel != nullptr) {
		ModelRelease(EnemyBoss::s_pModel);
		EnemyBoss::s_pModel = nullptr;
	}
}

// シャドウマップ専用描画
void EnemyBoss::DrawShadow() const
{
	if (s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDrawShadow(s_pModel, mtxWorld);
	}
}

// プレイヤーの方向を向く処理（Y軸回転）
void EnemyBoss::LookAtPlayer(double elapsed_time)
{
	const XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
	
	// プレイヤーへの方向ベクトルを計算
	float dx = playerPos.x - m_Position.x;
	float dz = playerPos.z - m_Position.z;
	
	// 目標の回転角度を計算（Y軸回転）
	float targetRotationY = atan2f(dx, dz);
	
	// 現在の回転角度と目標角度の差を計算
	float diff = targetRotationY - m_Rotation.y;
	
	// 角度の差を正規化
	while (diff > XM_PI) diff -= XM_2PI;
	while (diff < -XM_PI) diff += XM_2PI;
	
	// 回転速度に基づいて回転
	float maxRotation = TurnSpeed * static_cast<float>(elapsed_time);
	if (fabsf(diff) <= maxRotation)
	{
		// 目標角度に到達
		m_Rotation.y = targetRotationY;
	}
	else
	{
		// 徐々に回転
		if (diff > 0)
			m_Rotation.y += maxRotation;
		else
			m_Rotation.y -= maxRotation;
	}
	
	// 回転角度を正規化
	while (m_Rotation.y > XM_PI) m_Rotation.y -= XM_2PI;
	while (m_Rotation.y < -XM_PI) m_Rotation.y += XM_2PI;
}

// =======================================================

void EnemyBoss::EnemyBossStateIdle::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーが検知範囲に入ったら警戒状態へ遷移
	if (Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyBossStateAlert(m_pOwner));
	}
}

void EnemyBoss::EnemyBossStateIdle::Draw() const
{
	if (EnemyBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyBoss::s_pModel, mtxWorld);
	}

	// 球のコライダー可視化
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.5f, 0.0f, 1.0f }); // オレンジ色

#ifdef _DEBUG
	// 検知範囲を表示
	Sphere detectionSphere = { m_pOwner->m_Position, m_pOwner->m_DetectionRadius };
	DebugDraw_Sphere(detectionSphere, { 0.0f, 0.0f, 1.0f, 0.3f }); // 青（透明度低）
#endif
}

// =======================================================

void EnemyBoss::EnemyBossStateAlert::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーが検知範囲外に出たらIdle状態に戻る
	if (!Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyBossStateIdle(m_pOwner));
		return;
	}

	// プレイヤーの方向を向く
	m_pOwner->LookAtPlayer(elapsed_time);

	// プレイヤーが攻撃範囲に入ったら攻撃状態へ遷移
	if (Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_AttackRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyBossStateAttack(m_pOwner));
	}
}

void EnemyBoss::EnemyBossStateAlert::Draw() const
{
	if (EnemyBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyBoss::s_pModel, mtxWorld);
	}

	// 球のコライダー可視化
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 1.0f, 0.0f, 1.0f }); // 黄色（警戒中）

#ifdef _DEBUG
	// 検知範囲を表示
	Sphere detectionSphere = { m_pOwner->m_Position, m_pOwner->m_DetectionRadius };
	DebugDraw_Sphere(detectionSphere, { 1.0f, 1.0f, 0.0f, 0.3f }); // 黄（透明度低）
#endif
}

// =======================================================

void EnemyBoss::EnemyBossStateAttack::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーの方向を向き続ける
	m_pOwner->LookAtPlayer(elapsed_time);

	// 発射タイマーを更新
	m_pOwner->m_FireTimer += elapsed_time;

	// 発射間隔ごとにプレイヤーに向けて弾丸を発射
	if (m_pOwner->m_FireTimer >= EnemyBoss::FireInterval)
	{
		m_pOwner->m_FireTimer = 0.0;

		// プレイヤーが攻撃範囲内にいる場合のみ発射
		const XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
		XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_pOwner->m_Position);
		float distance = XMVectorGetX(XMVector3Length(toPlayer));

		if (distance <= m_pOwner->m_AttackRadius)
		{
			// 弾丸を発射
			EnemyBullet_Create(m_pOwner->m_Position, playerPos, EnemyBoss::BulletSpeed);
			// 発射音を再生
			if (g_EnebullSeId != -1) {
				PlayAudio(g_EnebullSeId, false);
			}
		}
	}

	// プレイヤーが検知範囲外に出たらIdle状態に戻る
	if (!Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		m_AccumulatedTime += elapsed_time;

		// 3秒間範囲外なら待機状態へ
		if (m_AccumulatedTime >= 3.0)
		{
			m_pOwner->ChangeState(new EnemyBossStateIdle(m_pOwner));
		}
	}
	else
	{
		m_AccumulatedTime = 0.0;
	}
}

void EnemyBoss::EnemyBossStateAttack::Draw() const
{
	if (EnemyBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyBoss::s_pModel, mtxWorld);
	}

	// 球のコライダー可視化
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色（攻撃中）

#ifdef _DEBUG
	// 攻撃範囲を表示
	Sphere attackSphere = { m_pOwner->m_Position, m_pOwner->m_AttackRadius };
	DebugDraw_Sphere(attackSphere, { 1.0f, 0.0f, 0.0f, 0.2f }); // 赤（透明度低）
#endif
}

