// ----------------------------------------------------
// ボス敵制御 [enemy_boss.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#ifndef ENEMY_BOSS_H
#define ENEMY_BOSS_H

#include "Enemy.h"
#include <DirectXMath.h>
#include "model.h"

// EnemyBoss用のモデルリソース管理
void EnemyBoss_InitializeResource();
void EnemyBoss_FinalizeResource();

class EnemyBoss : public Enemy
{
private:
	DirectX::XMFLOAT3 m_Position{};
	DirectX::XMFLOAT3 m_Rotation{};      // 回転角度（Y軸回転でプレイヤーの方向を向く）
	float m_DetectionRadius{ 50.0f };    // プレイヤー検知範囲
	float m_AttackRadius{ 50.0f };    // 攻撃可能範囲
	int m_Hp{ 300 };      // ボスHP
	double m_FireTimer{ 0.0 };  // 発射タイマー
	static constexpr double FireInterval = 5.0; // 発射間隔（秒）
	static constexpr float BulletSpeed = 5.0f;  // 弾丸速度
	static constexpr float TurnSpeed = 3.0f;    // 回転速度（ラジアン/秒）

	// 静的モデルリソース
	static MODEL* s_pModel;

	friend void EnemyBoss_InitializeResource();
	friend void EnemyBoss_FinalizeResource();

public:
	EnemyBoss(const DirectX::XMFLOAT3& position) : m_Position(position), m_Rotation{0.0f, 0.0f, 0.0f}
	{
		ChangeState(new EnemyBossStateIdle(this));
	}

	void Damage(int damage) override {
		m_Hp -= damage;
	}

	bool IsDestroy() const override {
		return m_Hp <= 0;
	}

	Sphere GetCollision() const override { return { m_Position, 2.4f }; } // ボスは大きい
	
	bool IsBoss() const override { return true; } // ボスであることを示す
	
	DirectX::XMFLOAT3 GetPosition() const override { return m_Position; } // 位置取得
	
	// シャドウマップ専用描画
	void DrawShadow() const override;

private:
	// プレイヤーの方向を向く処理（Y軸回転）
	void LookAtPlayer(double elapsed_time);
	
	// 待機状態（プレイヤーを待つ）
	class EnemyBossStateIdle : public State
	{
	private:
		EnemyBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyBossStateIdle(EnemyBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	// 警戒状態（プレイヤーの方向を向く）
	class EnemyBossStateAlert : public State
	{
	private:
		EnemyBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyBossStateAlert(EnemyBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	// 攻撃状態（プレイヤーに弾丸を発射）
	class EnemyBossStateAttack : public State
	{
	private:
		EnemyBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyBossStateAttack(EnemyBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};
};

#endif // ENEMY_BOSS_H
