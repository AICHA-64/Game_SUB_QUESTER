// ----------------------------------------------------
// ラスボス敵管理 [enemy_final_boss.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-03-02
// Version: 1.0
// ----------------------------------------------------
#ifndef ENEMY_FINAL_BOSS_H
#define ENEMY_FINAL_BOSS_H

#include "Enemy.h"
#include <DirectXMath.h>
#include "model.h"

// EnemyFinalBoss用のモデルリソース管理
void EnemyFinalBoss_InitializeResource();
void EnemyFinalBoss_FinalizeResource();

class EnemyFinalBoss : public Enemy
{
private:
	DirectX::XMFLOAT3 m_Position{};
	DirectX::XMFLOAT3 m_Rotation{};      // 回転角度(Y軸回転でプレイヤーの方を向く)
	float m_DetectionRadius{ 100.0f };   // プレイヤー検知範囲(通常ボスの2倍)
	float m_AttackRadius{ 100.0f };      // 攻撃可能範囲(通常ボスの2倍)
	int m_Hp{ 1000 };        // ラスボスHP(通常ボスの数倍)
	int m_MaxHp{ 1000 };     // 最大HP
	double m_FireTimer{ 0.0 };    // 発射タイマー
	
	// 弾幕攻撃用タイマー（レベル2以降）
	double m_BarrageTimer{ 0.0 };
	double m_BarrageInterval{ 8.0 }; // 弾幕発射間隔（レベルで変動）
	
	// レベルに応じた動的パラメータ
	int m_BossLevel{ 0 };     // ラスボスのレベル
	double m_FireInterval{ 5.0 };    // 発射間隔（レベルで変動）
	static constexpr double BaseFireInterval = 5.0;  // 基本発射間隔
	static constexpr double MinFireInterval = 0.5;   // 最小発射間隔（上限）
	static constexpr double FireIntervalDecrement = 0.5; // レベル毎の減少量
	
	// 弾幕パラメータ
	static constexpr double BaseBarrageInterval = 8.0;  // 基本弾幕間隔
	static constexpr double MinBarrageInterval = 3.0;   // 最小弾幕間隔
	static constexpr double BarrageIntervalDecrement = 0.5; // レベル毎の減少量
	
	static constexpr float BulletSpeed = 7.0f;   // 弾速(やや速め)
	static constexpr float BarrageBulletSpeed = 5.0f; // 弾幕の弾速（追従しない）
	static constexpr float TurnSpeed = 2.0f;  // 回転速度(ラジアン/秒)
	static constexpr float Scale = 1.5f; // 通常ボスの1.5倍の大きさ
	static constexpr int BaseHitsToDefeat = 10;  // 基本撃破回数

	// 静的モデルリソース
	static MODEL* s_pModel;

	friend void EnemyFinalBoss_InitializeResource();
	friend void EnemyFinalBoss_FinalizeResource();

public:
	EnemyFinalBoss(const DirectX::XMFLOAT3& position);

	void Damage(int damage) override {
		m_Hp -= damage;
	}

	bool IsDestroy() const override {
		return m_Hp <= 0;
	}

	Sphere GetCollision() const override { return { m_Position, 6.0f }; }
	
	bool IsBoss() const override { return true; }
	bool IsFinalBoss() const override { return true; } // ラスボスであることを示す
	
	DirectX::XMFLOAT3 GetPosition() const override { return m_Position; }
	
	// HP取得(UI表示用)
	int GetHp() const { return m_Hp; }
	int GetMaxHp() const { return m_MaxHp; }
	
	// レベル取得
	int GetBossLevel() const { return m_BossLevel; }
	
	// シャドウマップ専用描画
	void DrawShadow() const override;

private:
	// プレイヤーの方を向く(Y軸回転)
	void LookAtPlayer(double elapsed_time);
	
	// レベルに応じたパラメータを設定
	void SetupLevelParameters();
	
	// 弾幕攻撃を発射（レベル2以降）
	void FireBarrage();
	
	// 待機状態(プレイヤーを待つ)
	class EnemyFinalBossStateIdle : public State
	{
	private:
		EnemyFinalBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyFinalBossStateIdle(EnemyFinalBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	// 警戒状態(プレイヤーの方を向く)
	class EnemyFinalBossStateAlert : public State
	{
	private:
		EnemyFinalBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyFinalBossStateAlert(EnemyFinalBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	// 攻撃状態(プレイヤーに弾を発射)
	class EnemyFinalBossStateAttack : public State
	{
	private:
		EnemyFinalBoss* m_pOwner{};
		double m_AccumulatedTime{};

	public:
		EnemyFinalBossStateAttack(EnemyFinalBoss* pOwner)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};
};

#endif // ENEMY_FINAL_BOSS_H
