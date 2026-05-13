// ----------------------------------------------------
// エネミー制御 [Enemy.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#include "Enemy.h"
#include "enemy_normal.h"
#include "enemy_boss.h"
#include "enemy_final_boss.h"
#include "player.h"
#include "shot_counter.h"

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <vector>
#include <memory>
#include "bullet_hit_effect.h"

void Enemy::Update(double elapsed_time)
{
	if (m_pState) {
		m_pState->Update(elapsed_time);
	}
}

void Enemy::Draw() const
{
	if (m_pState) {
		m_pState->Draw();
	}
}

void Enemy::UpdateState()
{
	if (m_pNextState) {
		m_pState = std::move(m_pNextState);
	}
}

void Enemy::ChangeState(State* pNext) // モードの切り替え
{
	m_pNextState.reset(pNext);
}

static constexpr int MAX_ENEMY = 32; // ボス含めて増やす

static std::vector<std::unique_ptr<Enemy>> g_Enemys;
static int g_TotalBossCount{ 0 }; // 初期ボス数を記録

// ラスボス戦関連
static int g_FinalBossChance{ 0 }; // ラスボス戦確率(0-100%)
static bool g_IsFinalBossMode{ false }; // ラスボス戦モードフラグ
static constexpr int FINAL_BOSS_CHANCE_PER_KILL = 50; // EnemyNormal1体撃破ごとの確率上昇
static constexpr int FINAL_BOSS_CHANCE_MAX = 100; // 最大確率

// スポーン管理
static double g_SpawnAccum{ 0.0 };
static constexpr double SPAWN_INTERVAL = 0.1;
static constexpr double FINAL_BOSS_SPAWN_INTERVAL = 0.5; // ラスボス戦用(より遅い)
static constexpr float SPAWN_Y = 1.0f;
static constexpr float MIN_PLAYER_DISTANCE = 25.0f; // プレイヤーから最低離れる距離
static constexpr int MAX_SPAWN_ATTEMPTS = 10; // スポーン位置を探す最大試行回数
static constexpr float BOSS_SPAWN_RADIUS_MIN = 10.0f; // ボス周囲のスポーン最小半径
static constexpr float BOSS_SPAWN_RADIUS_MAX = 30.0f; // ボス周囲のスポーン最大半径
static constexpr float SPAWN_Y_OFFSET_MIN = -7.0f;    // ボスY座標からの最小オフセット(より深く)
static constexpr float SPAWN_Y_OFFSET_MAX = 5.0f;     // ボスY座標からの最大オフセット
static int g_NextBossIndex = 0; // 次にスポーンさせるボスのインデックス(ばらけるように)

// ラスボス戦用スポーン設定
static constexpr float FINAL_BOSS_SPAWN_RADIUS_MIN = 15.0f;
static constexpr float FINAL_BOSS_SPAWN_RADIUS_MAX = 40.0f;
static constexpr int MAX_GUARD_ENEMIES = 8; // ラスボス戦での最大ガード敵数
static constexpr float FINAL_BOSS_SPAWN_Y_MIN = -7.0f;  // ラスボス戦用Y座標最小値(水面より少し下)
static constexpr float FINAL_BOSS_SPAWN_Y_MAX = -1.0f;  // ラスボス戦用Y座標最大値(水面付近)
static constexpr double GUARD_RESPAWN_TIME = 7.0; // ガード敵の復活時間（秒）

// ガード敵の復活管理
struct GuardRespawnInfo {
	double timer;
	DirectX::XMFLOAT3 spawnPos;
	bool isActive;
};
static GuardRespawnInfo g_GuardRespawnQueue[MAX_GUARD_ENEMIES]{};

// ボス破壊時のパーティクル設定
static constexpr int BOSS_EXPLOSION_PARTICLE_COUNT = 42;
static constexpr float BOSS_EXPLOSION_RADIUS = 5.0f;
static constexpr float BOSS_EXPLOSION_Y_OFFSET = 3.0f;


void Enemy_Initialize()
{
	g_TotalBossCount = 0;
	g_SpawnAccum = 0.0;
	g_NextBossIndex = 0;
	// ラスボス戦モードはリセットしない(game.cppで設定される)
	// seed random
	srand((unsigned)time(nullptr));
	
	// 復活キューを初期化
	for (int i = 0; i < MAX_GUARD_ENEMIES; i++) {
		g_GuardRespawnQueue[i].timer = 0.0;
		g_GuardRespawnQueue[i].isActive = false;
	}
	
	EnemyNormal_InitializeResource();
	
	EnemyNormal_InitializeAudio();
	
	EnemyBoss_InitializeResource();
	
	EnemyFinalBoss_InitializeResource();
}

void Enemy_Finalize()
{
	g_Enemys.clear();
	
	EnemyNormal_FinalizeAudio();
	
	EnemyNormal_FinalizeResource();
	
	EnemyBoss_FinalizeResource();
	
	EnemyFinalBoss_FinalizeResource();
}

// 生存しているボスの位置を取得する
static std::vector<DirectX::XMFLOAT3> GetAliveBossPositions()
{
	std::vector<DirectX::XMFLOAT3> positions;
	for (int i = 0; i < g_Enemys.size(); i++) {
		if (g_Enemys[i]->IsBoss()) {
			positions.push_back(g_Enemys[i]->GetPosition());
		}
	}
	return positions;
}

#ifdef _DEBUG
// 現在のガード敵の数をカウント
static int GetGuardEnemyCount()
{
	int count = 0;
	for (int i = 0; i < g_Enemys.size(); i++) {
		// EnemyNormalでガードモードのものをカウント
		EnemyNormal* normal = dynamic_cast<EnemyNormal*>(g_Enemys[i].get());
		if (normal && normal->IsGuardMode()) {
			count++;
		}
	}
	return count;
}
#endif

static int SpawnGuardEnemyWithRespawn(const DirectX::XMFLOAT3& bossPos)
{
	if (g_Enemys.size() >= MAX_ENEMY) return -1;
	
	const DirectX::XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
	
	// ラスボス周囲の有効なスポーン位置を探す
	bool validPositionFound = false;
	DirectX::XMFLOAT3 spawnPos;
	
	for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
		// ラスボス周囲のランダムな角度と距離を生成
		float angle = (float)rand() / (float)RAND_MAX * 2.0f * 3.14159265f;
		float radius = FINAL_BOSS_SPAWN_RADIUS_MIN + (float)rand() / (float)RAND_MAX * (FINAL_BOSS_SPAWN_RADIUS_MAX - FINAL_BOSS_SPAWN_RADIUS_MIN);
		
		// ラスボス位置を中心としたスポーン位置を計算(XZ平面)
		spawnPos.x = bossPos.x + cosf(angle) * radius;
		spawnPos.z = bossPos.z + sinf(angle) * radius;
		
		// Y座標は固定範囲内でランダム(地面より上、水面付近)
		spawnPos.y = FINAL_BOSS_SPAWN_Y_MIN + (float)rand() / (float)RAND_MAX * (FINAL_BOSS_SPAWN_Y_MAX - FINAL_BOSS_SPAWN_Y_MIN);
		
		// プレイヤーとの距離を計算
		float dx = spawnPos.x - playerPos.x;
		float dz = spawnPos.z - playerPos.z;
		float distanceSquared = dx * dx + dz * dz;
		
		// 最低距離より離れていれば有効な位置
		if (distanceSquared >= MIN_PLAYER_DISTANCE * MIN_PLAYER_DISTANCE) {
			validPositionFound = true;
			break;
		}
	}
	
	// 有効な位置が見つかった場合のみスポーン
	if (validPositionFound) {
		if (g_Enemys.size() < MAX_ENEMY) {
			auto guardEnemy = std::make_unique<EnemyNormal>(spawnPos);
			guardEnemy->SetGuardMode(true); // ガードモードを有効化
			g_Enemys.push_back(std::move(guardEnemy));
			
			// 空いている復活キューのスロットを探してスポーン位置を記憶
			for (int i = 0; i < MAX_GUARD_ENEMIES; i++) {
				if (!g_GuardRespawnQueue[i].isActive) {
					g_GuardRespawnQueue[i].spawnPos = spawnPos;
					g_GuardRespawnQueue[i].isActive = true;
					g_GuardRespawnQueue[i].timer = 0.0;
					return i; // スロット番号を返す
				}
			}
		}
	}
	
	return -1;
}

void Enemy_Update(double elapsed_time)
{
	// ラスボス戦モードかどうかでスポーン処理が異なる
	if (g_IsFinalBossMode) {
		// ガード敵の復活処理
		for (int i = 0; i < MAX_GUARD_ENEMIES; i++) {
			if (g_GuardRespawnQueue[i].isActive && g_GuardRespawnQueue[i].timer > 0.0) {
				g_GuardRespawnQueue[i].timer -= elapsed_time;
				
				// タイマーが0になったら復活
				if (g_GuardRespawnQueue[i].timer <= 0.0) {
					if (g_Enemys.size() < MAX_ENEMY) {
						DirectX::XMFLOAT3 spawnPos = g_GuardRespawnQueue[i].spawnPos;
						auto guardEnemy = std::make_unique<EnemyNormal>(spawnPos);
						guardEnemy->SetGuardMode(true);
						g_Enemys.push_back(std::move(guardEnemy));
						
						// タイマーをリセット（次回の復活用）
						g_GuardRespawnQueue[i].timer = 0.0;
					}
				}
			}
		}
	} else {
		// 通常モード:スポーン処理
		g_SpawnAccum += elapsed_time;
		while (g_SpawnAccum >= SPAWN_INTERVAL) {
			g_SpawnAccum -= SPAWN_INTERVAL;
			if (g_Enemys.size() < MAX_ENEMY) {
				// 生存しているボスの位置を取得
				std::vector<DirectX::XMFLOAT3> bossPositions = GetAliveBossPositions();
				
				// ボスがいない場合はスポーンしない
				if (bossPositions.empty()) {
					continue;
				}
				
				// 次にスポーンさせるボスを選択
				int bossCount = static_cast<int>(bossPositions.size());
				int targetBossIdx = g_NextBossIndex % bossCount;
				g_NextBossIndex++; // 次回は別のボスの周囲にスポーン
				
				const DirectX::XMFLOAT3& bossPos = bossPositions[targetBossIdx];
				const DirectX::XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
				
				// ボス周囲の有効なスポーン位置を探す
				bool validPositionFound = false;
				DirectX::XMFLOAT3 spawnPos;
				
				for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
					// ボス周囲のランダムな角度と距離を生成
					float angle = (float)rand() / (float)RAND_MAX * 2.0f * 3.14159265f;
					float radius = BOSS_SPAWN_RADIUS_MIN + (float)rand() / (float)RAND_MAX * (BOSS_SPAWN_RADIUS_MAX - BOSS_SPAWN_RADIUS_MIN);
					
					// Y軸のオフセットをランダムに生成(ボスのY座標を基準)
					float yOffset = SPAWN_Y_OFFSET_MIN + (float)rand() / (float)RAND_MAX * (SPAWN_Y_OFFSET_MAX - SPAWN_Y_OFFSET_MIN);
					
					// ボス位置を中心としたスポーン位置を計算
					spawnPos.x = bossPos.x + cosf(angle) * radius;
					spawnPos.y = bossPos.y + yOffset;
					spawnPos.z = bossPos.z + sinf(angle) * radius;
					
					// プレイヤーとの距離を計算
					float dx = spawnPos.x - playerPos.x;
					float dz = spawnPos.z - playerPos.z;
					float distanceSquared = dx * dx + dz * dz;
					
					// 最低距離より離れていれば有効な位置
					if (distanceSquared >= MIN_PLAYER_DISTANCE * MIN_PLAYER_DISTANCE) {
						validPositionFound = true;
						break;
					}
				}
				
				// 有効な位置が見つかった場合のみスポーン
				if (validPositionFound) {
					Enemy_Create(spawnPos);
				}
			}
		}
	}

	// Stateの切り替え
	for (int i = 0; i < g_Enemys.size(); i++) {
		g_Enemys[i]->UpdateState();
	}

	// 更新処理
	for (int i = 0; i < g_Enemys.size(); i++) {
		g_Enemys[i]->Update(elapsed_time);
	}

	// 破棄処理
	for (int i = static_cast<int>(g_Enemys.size()) - 1; i >= 0; i--) {
		if (g_Enemys[i]->IsDestroy()) {
			// ガード敵が倒された場合は復活タイマーを開始
			if (g_IsFinalBossMode) {
				EnemyNormal* normal = dynamic_cast<EnemyNormal*>(g_Enemys[i].get());
				if (normal && normal->IsGuardMode()) {
					// この敵のスポーン位置を持つキューを探す
					DirectX::XMFLOAT3 enemyPos = g_Enemys[i]->GetPosition();
					for (int j = 0; j < MAX_GUARD_ENEMIES; j++) {
						if (g_GuardRespawnQueue[j].isActive) {
							// スポーン位置が近い場合、このスロットに対応すると判断
							DirectX::XMFLOAT3& queuePos = g_GuardRespawnQueue[j].spawnPos;
							float dx = queuePos.x - enemyPos.x;
							float dz = queuePos.z - enemyPos.z;
							float distSq = dx * dx + dz * dz;
							
							if (distSq < 5.0f * 5.0f) { // 5m以内なら同一スポット
								// 復活タイマーを開始
								g_GuardRespawnQueue[j].timer = GUARD_RESPAWN_TIME;
								break;
							}
						}
					}
				}
			}
			
			// EnemyNormal の撃破数をカウント & ラスボス戦確率上昇
			if (dynamic_cast<EnemyNormal*>(g_Enemys[i].get()) != nullptr) {
				ShotCounter_Add();
				// ラスボス戦モードでない場合のみ確率上昇
				if (!g_IsFinalBossMode) {
					Enemy_AddFinalBossChance(FINAL_BOSS_CHANCE_PER_KILL);
				}
			}

			// ボスが破壊された場合は複数のパーティクルを生成して爆発表現
			if (g_Enemys[i]->IsBoss()) {
				DirectX::XMFLOAT3 bossPos = g_Enemys[i]->GetPosition();
				
				// ラスボスの場合はより派手な爆発
				int particleCount = g_Enemys[i]->IsFinalBoss() ? 
					BOSS_EXPLOSION_PARTICLE_COUNT * 3 : BOSS_EXPLOSION_PARTICLE_COUNT;
				float explosionRadius = g_Enemys[i]->IsFinalBoss() ? 
					BOSS_EXPLOSION_RADIUS * 2.0f : BOSS_EXPLOSION_RADIUS;
				
				for (int p = 0; p < particleCount; ++p) {
					float angle = (float)rand() / (float)RAND_MAX * 2.0f * 3.14159265f;
					float radius = (float)rand() / (float)RAND_MAX * explosionRadius;
					float yOff = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * BOSS_EXPLOSION_Y_OFFSET;
					DirectX::XMFLOAT3 partPos{
						bossPos.x + cosf(angle) * radius,
						bossPos.y + yOff,
						bossPos.z + sinf(angle) * radius
					};
					BulletHitEffect_Create(partPos);
				}
			}

			g_Enemys[i] = std::move(g_Enemys.back());
			g_Enemys.pop_back();
		}
	}
}

void Enemy_Draw()
{
	for (int i = 0; i < g_Enemys.size(); i++) {
		g_Enemys[i]->Draw();
	}
}

// シャドウマップ専用描画関数(深度のみ)
void Enemy_DrawShadow()
{
	for (int i = 0; i < g_Enemys.size(); i++) {
		g_Enemys[i]->DrawShadow();
	}
}

void Enemy_Create(const DirectX::XMFLOAT3& position)
{
	if (g_Enemys.size() >= MAX_ENEMY) return;
	g_Enemys.push_back(std::make_unique<EnemyNormal>(position));
}

void Enemy_CreateBoss(const DirectX::XMFLOAT3& position)
{
	if (g_Enemys.size() >= MAX_ENEMY) return;
	g_Enemys.push_back(std::make_unique<EnemyBoss>(position));
	g_TotalBossCount++; // ボス生成時にカウントアップ
}

void Enemy_CreateFinalBoss(const DirectX::XMFLOAT3& position)
{
	if (g_Enemys.size() >= MAX_ENEMY) return;
	g_Enemys.push_back(std::make_unique<EnemyFinalBoss>(position));
	g_TotalBossCount++;
	
	// ラスボス生成時、最大数までガード敵を一気にスポーン
	for (int i = 0; i < MAX_GUARD_ENEMIES; i++) {
		SpawnGuardEnemyWithRespawn(position);
	}
	
#ifdef _DEBUG
	char buf[128];
	sprintf_s(buf, "FinalBoss created with %d guard enemies\n", GetGuardEnemyCount());
	OutputDebugStringA(buf);
#endif
}

int Enemy_GetEnemyCount()
{
	return static_cast<int>(g_Enemys.size());
}

Enemy* Enemy_GetEnemy(int index)
{
	return g_Enemys[index].get();
}

bool Enemy_AreAllBossesDefeated()
{
	// 初期ボス数が0の場合は常にfalse(ボスがいなければクリア条件にならない)
	if (g_TotalBossCount == 0) {
		return false;
	}
	
	// 現在生きているボスの数をカウント
	int aliveBossCount = 0;
	for (int i = 0; i < g_Enemys.size(); i++) {
		if (g_Enemys[i]->IsBoss()) {
			aliveBossCount++;
		}
	}
	
	// 生きているボスが0ならすべて倒されている
	return aliveBossCount == 0;
}

bool Enemy_IsAnyBossDefeated()
{
	// 初期ボス数が0の場合は常にfalse(ボスがいなければクリア条件にならない)
	if (g_TotalBossCount == 0) {
		return false;
	}
	
	// 現在生存しているボスの数をカウント
	int aliveBossCount = Enemy_GetAliveBossCount();
	
	// 初期ボス数より現在の生存数が少なければ、少なくとも1体は撃破されている
	return aliveBossCount < g_TotalBossCount;
}

int Enemy_GetAliveBossCount()
{
	int aliveBossCount = 0;
	for (int i = 0; i < g_Enemys.size(); i++) {
		if (g_Enemys[i]->IsBoss()) {
			aliveBossCount++;
		}
	}
	return aliveBossCount;
}

// ラスボス戦確率を加算
void Enemy_AddFinalBossChance(int percent)
{
	g_FinalBossChance += percent;
	if (g_FinalBossChance > FINAL_BOSS_CHANCE_MAX) {
		g_FinalBossChance = FINAL_BOSS_CHANCE_MAX;
	}
}

// 現在のラスボス戦確率を取得
int Enemy_GetFinalBossChance()
{
	return g_FinalBossChance;
}

// ラスボス戦確率をリセット
void Enemy_ResetFinalBossChance()
{
	g_FinalBossChance = 0;
}

// ラスボス戦に突入するか判定
bool Enemy_RollFinalBossBattle()
{
	int roll = rand() % 100; // 0-99
	return roll < g_FinalBossChance;
}

// ラスボス戦モードを設定
void Enemy_SetFinalBossMode(bool isFinalBoss)
{
	g_IsFinalBossMode = isFinalBoss;
}

// ラスボス戦モードかどうか
bool Enemy_IsFinalBossMode()
{
	return g_IsFinalBossMode;
}

// ラスボスが倒されたか
bool Enemy_IsFinalBossDefeated()
{
	if (!g_IsFinalBossMode) {
		return false;
	}
	// ラスボス戦モードで全ボスが倒されたらラスボス撃破
	return Enemy_AreAllBossesDefeated();
}

#ifdef _DEBUG
void Enemy_KillAllBosses()
{
	// すべてのボスに大ダメージを与えて倒す
	for (int i = 0; i < g_Enemys.size(); i++) {
		if (g_Enemys[i]->IsBoss()) {
			g_Enemys[i]->Damage(99999); // 即死ダメージ
		}
	}
}

void Enemy_SetFinalBossChance(int percent)
{
	g_FinalBossChance = percent;
	if (g_FinalBossChance < 0) g_FinalBossChance = 0;
	if (g_FinalBossChance > FINAL_BOSS_CHANCE_MAX) g_FinalBossChance = FINAL_BOSS_CHANCE_MAX;
}
#endif
