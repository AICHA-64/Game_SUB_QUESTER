// ----------------------------------------------------
// 通常の敵制御 [enemy_normal.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#ifndef ENEMY_NORMAL_H
#define ENEMY_NORMAL_H

#include "Enemy.h"
#include <DirectXMath.h>
#include "texture.h"
#include "route_search.h"
#include "model.h"
#include <cstdlib>

// EnemyNormal用のモデルリソース管理
void EnemyNormal_InitializeResource();
void EnemyNormal_FinalizeResource();

// EnemyNormal用のオーディオリソース管理
void EnemyNormal_InitializeAudio();
void EnemyNormal_FinalizeAudio();

class EnemyNormal : public Enemy
{
private:
	DirectX::XMFLOAT3 m_Position{};
	float m_DetectionRadius{20.0f};
	float m_ExplosionRadius{4.0f};  // 爆発範囲の半径
	int m_Hp{ 100 };
	bool m_IsGuardMode{ false }; // ガードモードフラグ(ラスボス戦用)
	bool m_IsIntercepting{ false }; // インターセプト中フラグ
	
	// 静的モデルリソース
	static MODEL* s_pModel;
	
	// 静的オーディオリソース
	static int s_PonSeId;      // カウントダウン音
	static int s_ExplosionSeId; // 爆発音
	
	friend void EnemyNormal_InitializeResource();
	friend void EnemyNormal_FinalizeResource();
	friend void EnemyNormal_InitializeAudio();
	friend void EnemyNormal_FinalizeAudio();
	
	// 地形に合わせて位置を調整するヘルパー関数
	void AdjustPositionToTerrain();
	
	// 定数群
	static constexpr float COLLISION_RADIUS = 0.9f;
	static constexpr float RAY_CAST_HEIGHT = 100.0f;
	static constexpr float GROUND_OFFSET_Y = 0.5f;
	static constexpr float HORIZONTAL_PUSH_THRESHOLD = 0.01f;
	static constexpr int MAX_ADJUST_ATTEMPTS = 5;
	
public:
	EnemyNormal(const DirectX::XMFLOAT3& position);

	void Damage(int damage) override {
		m_Hp -= damage;
	}

	bool IsDestroy() const override {
		return m_Hp <= 0;
	}
	
	// シャドウマップ専用描画
	void DrawShadow() const override;

	Sphere GetCollision() const { return { m_Position, COLLISION_RADIUS }; }
	
	DirectX::XMFLOAT3 GetPosition() const override { return m_Position; } // 位置取得
	
	float GetExplosionRadius() const { return m_ExplosionRadius; } // 爆発半径取得
	
	// ガードモードを設定(ラスボス戦用)
	void SetGuardMode(bool isGuard);
	bool IsGuardMode() const { return m_IsGuardMode; }
	
	// インターセプト状態の管理
	bool IsIntercepting() const { return m_IsIntercepting; }
	void SetIntercepting(bool intercepting) { m_IsIntercepting = intercepting; }
	
	// インターセプト状態へ遷移(弾丸の位置を指定)
	void StartIntercept(const DirectX::XMFLOAT3& bulletPos);

private:
	class EnemyNormalStatePatrol : public State
	{
	private:
		EnemyNormal* m_pOwner{};
		float m_PointX{};
		float m_PointZ{};  // Z座標の基点を追加
		double g_AccumulatedTime{};
		
		// 地形衝突時の反転処理用
		float m_MoveDirection{ 1.0f };    // 移動方向(1.0f: 順方向, -1.0f: 逆方向)
		float m_CurrentSpeed{ 0.0f };       // 現在の移動速度(0から開始)
		float m_MaxSpeed{ 2.5f };           // 最大移動速度
		float m_Deceleration{ 6.0f };       // 減速力(衝突時)
		float m_Acceleration{ 3.0f };     // 加速力(通常時)
		bool m_IsDecelerating{ false }; // 減速中フラグ
		float m_TurnCooldown{ 0.0f }; // 反転後のクールダウン(連続反転防止)
		
		static constexpr float TURN_COOLDOWN_TIME = 0.5f;  // 反転後のクールダウン時間
		static constexpr float MIN_SPEED_THRESHOLD = 0.1f; // この速度以下で停止とみなす

	public:
		EnemyNormalStatePatrol(EnemyNormal* pOwner)
			: m_pOwner(pOwner)
			, m_PointX(pOwner->m_Position.x)
			, m_PointZ(pOwner->m_Position.z)
			, m_CurrentSpeed(0.0f)
			, m_MoveDirection(1.0f)
		{ }

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	class EnemyNormalStateChase : public State
	{
	private:
		EnemyNormal* m_pOwner{};
		double m_AccumulatedTime{};
		double m_ChaseTime{}; // 追跡時間を管理するメンバ変数を追加
		RouteData m_RouteData{};
		int m_TargetRouteIndex{};
		float m_PointX{};
		// ジッター付きの検索間隔
		double m_SearchInterval{2.0};
	public:
		EnemyNormalStateChase(EnemyNormal* pOwner)
			: m_pOwner(pOwner), m_PointX(pOwner->m_Position.x), m_AccumulatedTime(0.0), m_ChaseTime(0.0)
		{
			// 初期ジッターを付与(±0.5秒)
			double jitter = ((double)std::rand() / (double)RAND_MAX -0.5) *1.0; // [-0.5,0.5]
			m_SearchInterval =2.0 + jitter;
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};

	// 爆発カウントダウンステート
	class EnemyNormalStateExplosion : public State
	{
	private:
		EnemyNormal* m_pOwner{};
		double m_CountdownTime{3.0};  // 3秒カウントダウン
		int m_LastPlayedSecond{3};    // 最後にSEを再生した秒数
		bool m_HasExploded{false};    // 爆発済みフラグ

	public:
		EnemyNormalStateExplosion(EnemyNormal* pOwner)
			: m_pOwner(pOwner)
		{
		}

		void Update(double elapsed_time) override;
		void Draw() const override;
	};
	
	// ガード待機状態(ラスボス周囲で散らばって待機)
	class EnemyNormalStateGuardIdle : public State
	{
	private:
		EnemyNormal* m_pOwner{};
		double m_AccumulatedTime{};
		DirectX::XMFLOAT3 m_PatrolCenter{}; // 巡回の中心位置
		float m_PatrolAngle{ 0.0f }; // 巡回角度
		float m_PatrolRadius{ 5.0f }; // 巡回半径
		float m_PatrolSpeed{ 1.0f }; // 巡回速度
		
	public:
		EnemyNormalStateGuardIdle(EnemyNormal* pOwner);
		
		void Update(double elapsed_time) override;
		void Draw() const override;
	};
	
	// インターセプト状態(弾丸に当たりに行く)
	class EnemyNormalStateIntercept : public State
	{
	private:
		EnemyNormal* m_pOwner{};
		double m_AccumulatedTime{};
		DirectX::XMFLOAT3 m_TargetPosition{}; // 弾丸の予測位置
		float m_MoveSpeed{ 8.0f }; // 移動速度（ゆっくり移動）
		double m_TimeoutTimer{ 5.0 }; // タイムアウト(弾が消えた場合など)
		bool m_HasReachedTarget{ false }; // 目標地点に到達したか
		double m_StayTimer{ 2.0 }; // 目標地点での滞在時間
		
	public:
		EnemyNormalStateIntercept(EnemyNormal* pOwner, const DirectX::XMFLOAT3& targetPos)
			: m_pOwner(pOwner), m_AccumulatedTime(0.0), m_TargetPosition(targetPos),
			  m_HasReachedTarget(false), m_StayTimer(2.0)
		{
		}
		
		void Update(double elapsed_time) override;
		void Draw() const override;
	};
};


#endif // ENEMY_NORMAL_H
