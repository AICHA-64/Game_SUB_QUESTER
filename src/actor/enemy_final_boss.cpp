// ----------------------------------------------------
// ラスボス敵管理 [enemy_final_boss.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-03-02
// Version: 1.0
// ----------------------------------------------------
#include "enemy_final_boss.h"
#include "collision.h"
#include "player.h"
#include "enemy_bullet.h"
#include "debug_draw.h"
#include "Audio.h"
#include "player_upgrade.h"
#include <cmath>
using namespace DirectX;

static int g_FinalBossBulletSeId = -1;

MODEL* EnemyFinalBoss::s_pModel = nullptr;

void EnemyFinalBoss_InitializeResource()
{
	if (EnemyFinalBoss::s_pModel == nullptr) {
		// 通常ボスの2.5倍のサイズでロード (2.5f * 2.5 = 6.25f)
		EnemyFinalBoss::s_pModel = ModelLoad("resource/model/boss.fbx", 6.25f, false);
	}

	g_FinalBossBulletSeId = LoadAudio("resource/audio/enebul.wav");
}

void EnemyFinalBoss_FinalizeResource()
{
	if (EnemyFinalBoss::s_pModel != nullptr) {
		ModelRelease(EnemyFinalBoss::s_pModel);
		EnemyFinalBoss::s_pModel = nullptr;
	}
}

EnemyFinalBoss::EnemyFinalBoss(const DirectX::XMFLOAT3& position)
	: m_Position(position)
	, m_Rotation{0.0f, 0.0f, 0.0f}
{
	// 現在のラスボスレベルを取得
	m_BossLevel = PlayerUpgrade_GetFinalBossLevel();
	
	// レベルに応じたパラメータを設定
	SetupLevelParameters();
	
	ChangeState(new EnemyFinalBossStateIdle(this));
	
	// デバッグ出力
	char buf[256];
	sprintf_s(buf, "FinalBoss created: Level=%d, HP=%d, FireInterval=%.2f, BarrageInterval=%.2f\n", 
		m_BossLevel, m_MaxHp, m_FireInterval, m_BarrageInterval);
#ifdef _DEBUG
	OutputDebugStringA(buf);
#endif
}

void EnemyFinalBoss::SetupLevelParameters()
{
	// HPの設定: レベルごとに必要攻撃回数を1増やす
	// 1回の攻撃で100ダメージと仮定（プレイヤーの基本ダメージ）
	int hitsRequired = BaseHitsToDefeat + m_BossLevel;
	m_MaxHp = hitsRequired * 100; // 必要攻撃回数 * 基本ダメージ
	m_Hp = m_MaxHp;
	
	// 攻撃頻度の設定: レベルごとに発射間隔を短縮（下限あり）
	m_FireInterval = BaseFireInterval - (m_BossLevel * FireIntervalDecrement);
	if (m_FireInterval < MinFireInterval) {
		m_FireInterval = MinFireInterval;
	}
	
	// 弾幕間隔の設定: レベル2以降で有効、レベルが上がるほど頻繁に
	if (m_BossLevel >= 2) {
		m_BarrageInterval = BaseBarrageInterval - ((m_BossLevel - 2) * BarrageIntervalDecrement);
		if (m_BarrageInterval < MinBarrageInterval) {
			m_BarrageInterval = MinBarrageInterval;
		}
	}
}

void EnemyFinalBoss::FireBarrage()
{
	// レベルに応じた弾幕の密度を計算
	// レベル2: 8方向、レベル3: 12方向、レベル4: 16方向...
	int bulletCount = 8 + (m_BossLevel - 2) * 4;
	if (bulletCount > 32) bulletCount = 32; // 最大32方向
	
	// 円周上に等間隔で弾を配置
	float angleStep = XM_2PI / bulletCount;
	
	for (int i = 0; i < bulletCount; i++) {
		float angle = angleStep * i;
		
		// 水平方向のベクトルを計算
		XMFLOAT3 direction;
		direction.x = cosf(angle);
		direction.y = 0.0f; // 水平方向のみ
		direction.z = sinf(angle);
		
		// 発射位置（ボスの位置から少し離す）
		XMFLOAT3 spawnPos = m_Position;
		spawnPos.x += direction.x * 3.0f;
		spawnPos.z += direction.z * 3.0f;
		
		// 目標位置（進行方向に弾速分だけ離れた位置）
		XMFLOAT3 targetPos;
		targetPos.x = spawnPos.x + direction.x * BarrageBulletSpeed;
		targetPos.y = spawnPos.y;
		targetPos.z = spawnPos.z + direction.z * BarrageBulletSpeed;
		
		// 弾幕を発射（追従しない直進弾）
		EnemyBullet_Create(spawnPos, targetPos, BarrageBulletSpeed);
	}
	
	// 弾幕発射音を再生
	if (g_FinalBossBulletSeId != -1) {
		PlayAudio(g_FinalBossBulletSeId, false);
	}
	
	// デバッグ出力
	char buf[128];
	sprintf_s(buf, "Barrage fired: %d bullets at Level %d\n", bulletCount, m_BossLevel);
#ifdef _DEBUG
	OutputDebugStringA(buf);
#endif
}

// シャドウマップ専用描画
void EnemyFinalBoss::DrawShadow() const
{
	if (s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDrawShadow(s_pModel, mtxWorld);
	}
}

// プレイヤーの方を向く(Y軸回転)
void EnemyFinalBoss::LookAtPlayer(double elapsed_time)
{
	const XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
	
	// プレイヤーへの方向ベクトルを計算
	float dx = playerPos.x - m_Position.x;
	float dz = playerPos.z - m_Position.z;
	
	// 目標の回転角度を計算(Y軸回転)
	float targetRotationY = atan2f(dx, dz);
	
	// 現在の回転角度と目標角度の差分を計算
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
// 待機状態
// =======================================================

void EnemyFinalBoss::EnemyFinalBossStateIdle::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーが検知範囲に入ったら警戒状態へ遷移
	if (Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyFinalBossStateAlert(m_pOwner));
	}
}

void EnemyFinalBoss::EnemyFinalBossStateIdle::Draw() const
{
	if (EnemyFinalBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyFinalBoss::s_pModel, mtxWorld);
	}

	// 体のコライダー描画
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 0.8f, 0.0f, 0.8f, 1.0f }); // 紫色(ラスボス)

#ifdef _DEBUG
	// 検知範囲を表示
	Sphere detectionSphere = { m_pOwner->m_Position, m_pOwner->m_DetectionRadius };
	DebugDraw_Sphere(detectionSphere, { 0.5f, 0.0f, 0.5f, 0.2f }); // 紫(半透明度)
#endif
}

// =======================================================
// 警戒状態
// =======================================================

void EnemyFinalBoss::EnemyFinalBossStateAlert::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーが検知範囲外に出たらIdle状態に戻る
	if (!Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyFinalBossStateIdle(m_pOwner));
		return;
	}

	// プレイヤーの方を向く
	m_pOwner->LookAtPlayer(elapsed_time);

	// プレイヤーが攻撃範囲に入ったら攻撃状態へ遷移
	if (Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_AttackRadius },
		Player::GetInstance().GetPosition()))
	{
		m_pOwner->ChangeState(new EnemyFinalBossStateAttack(m_pOwner));
	}
}

void EnemyFinalBoss::EnemyFinalBossStateAlert::Draw() const
{
	if (EnemyFinalBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyFinalBoss::s_pModel, mtxWorld);
	}

	// 体のコライダー描画
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 1.0f, 1.0f }); // マゼンタ(警戒中)

#ifdef _DEBUG
	// 検知範囲を表示
	Sphere detectionSphere = { m_pOwner->m_Position, m_pOwner->m_DetectionRadius };
	DebugDraw_Sphere(detectionSphere, { 1.0f, 0.0f, 1.0f, 0.2f }); // マゼンタ(半透明度)
#endif
}

// =======================================================
// 攻撃状態
// =======================================================

void EnemyFinalBoss::EnemyFinalBossStateAttack::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;

	// プレイヤーの方を向き続ける
	m_pOwner->LookAtPlayer(elapsed_time);

	// 発射タイマーを更新
	m_pOwner->m_FireTimer += elapsed_time;

	// 発射間隔ごとにプレイヤーを狙って弾を発射
	if (m_pOwner->m_FireTimer >= m_pOwner->m_FireInterval)
	{
		m_pOwner->m_FireTimer = 0.0;

		// プレイヤーが攻撃範囲内にある場合のみ発射
		const XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
		XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_pOwner->m_Position);
		float distance = XMVectorGetX(XMVector3Length(toPlayer));

		if (distance <= m_pOwner->m_AttackRadius)
		{
			// 弾を発射
			EnemyBullet_Create(m_pOwner->m_Position, playerPos, EnemyFinalBoss::BulletSpeed);
			// 発射音を再生
			if (g_FinalBossBulletSeId != -1) {
				PlayAudio(g_FinalBossBulletSeId, false);
			}
		}
	}
	
	// 弾幕攻撃タイマーを更新（レベル2以降）
	if (m_pOwner->m_BossLevel >= 2) {
		m_pOwner->m_BarrageTimer += elapsed_time;
		
		// 弾幕間隔ごとに弾幕を発射
		if (m_pOwner->m_BarrageTimer >= m_pOwner->m_BarrageInterval) {
			m_pOwner->m_BarrageTimer = 0.0;
			m_pOwner->FireBarrage();
		}
	}

	// プレイヤーが検知範囲外に出たらIdle状態に戻る
	if (!Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition()))
	{
		// 3秒間範囲外ならば待機状態へ
		if (m_AccumulatedTime >= 3.0)
		{
			m_pOwner->ChangeState(new EnemyFinalBossStateIdle(m_pOwner));
		}
	}
	else
	{
		m_AccumulatedTime = 0.0;
	}
}

void EnemyFinalBoss::EnemyFinalBossStateAttack::Draw() const
{
	if (EnemyFinalBoss::s_pModel != nullptr) {
		XMMATRIX mtxRot = XMMatrixRotationY(m_pOwner->m_Rotation.y);
		XMMATRIX mtxTrans = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		XMMATRIX mtxWorld = mtxRot * mtxTrans;
		ModelDraw(EnemyFinalBoss::s_pModel, mtxWorld);
	}

	// 体のコライダー描画
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色(攻撃中)

#ifdef _DEBUG
	// 攻撃範囲を表示
	Sphere attackSphere = { m_pOwner->m_Position, m_pOwner->m_AttackRadius };
	DebugDraw_Sphere(attackSphere, { 1.0f, 0.0f, 0.0f, 0.15f }); // 赤(半透明度)
#endif
}

