// ----------------------------------------------------
// エネミー制御 [Enemy.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#ifndef ENEMY_H
#define ENEMY_H

#include <memory>
#include <DirectXMath.h>
#include "collision.h"

class Enemy 
{
protected:
	class State 
	{
	public:
		virtual ~State() = default;
		virtual void Update(double elapsed_time) = 0;
		virtual void Draw() const = 0;
	};

private:
	std::unique_ptr<State> m_pState;
	std::unique_ptr<State> m_pNextState;

public:
	virtual ~Enemy() = default;
	virtual void Update(double elapsed_time);
	virtual void Draw() const;
	virtual void DrawShadow() const {} // シャドウマップ専用描画(デフォルト空)
	void UpdateState();
	virtual void Damage(int) {}
	virtual bool IsDestroy() const = 0;
	virtual Sphere GetCollision() const { return {}; }
	virtual bool IsBoss() const { return false; } // ボスかどうかを判定
	virtual bool IsFinalBoss() const { return false; } // ラスボスかどうかを判定
	virtual DirectX::XMFLOAT3 GetPosition() const { return {}; } // 位置取得

protected:
	void ChangeState(State* pNext);
};

void Enemy_Initialize();
void Enemy_Finalize();
void Enemy_Update(double elapsed_time);
void Enemy_Draw();

// シャドウマップ専用描画関数(深度のみ)
void Enemy_DrawShadow();

void Enemy_Create(const DirectX::XMFLOAT3& position);
void Enemy_CreateBoss(const DirectX::XMFLOAT3& position); // ボスエネミー生成
void Enemy_CreateFinalBoss(const DirectX::XMFLOAT3& position); // ラスボス生成
int Enemy_GetEnemyCount();
Enemy* Enemy_GetEnemy(int index);
bool Enemy_AreAllBossesDefeated(); // すべてのボスが倒されたかチェック
bool Enemy_IsAnyBossDefeated(); // ボスを1体でも撃破したかチェック
int Enemy_GetAliveBossCount(); // 生存しているボスの数を取得

// ラスボス戦関連
void Enemy_AddFinalBossChance(int percent); // ラスボス戦確率を加算(EnemyNormal撃破時に呼ぶ)
int Enemy_GetFinalBossChance(); // 現在のラスボス戦確率を取得(0-100)
void Enemy_ResetFinalBossChance(); // ラスボス戦確率をリセット
bool Enemy_RollFinalBossBattle(); // ラスボス戦に突入するか判定(確率でtrue/false)
void Enemy_SetFinalBossMode(bool isFinalBoss); // ラスボス戦モードを設定
bool Enemy_IsFinalBossMode(); // ラスボス戦モードかどうか
bool Enemy_IsFinalBossDefeated(); // ラスボスが倒されたか

#ifdef _DEBUG
void Enemy_KillAllBosses(); // デバッグ用:すべてのボスを倒す
void Enemy_SetFinalBossChance(int percent); // デバッグ用:確率を直接設定
#endif

#endif // ENEMY_H
