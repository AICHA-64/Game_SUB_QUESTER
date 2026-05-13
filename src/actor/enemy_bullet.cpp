// ----------------------------------------------------
// エネミー用弾丸の管理 [enemy_bullet.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#include "enemy_bullet.h"
#include "player.h"
#include "trajectory3d.h"
#include "debug_draw.h"
#include "model.h"
#include "map.h"
#include "terrain_collision.h"
#include "bullet_hit_effect.h" // added: play explosion on expiration
using namespace DirectX;

class EnemyBullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	float m_speed{};
	double m_accumulated_time{ 0.0 };
	double m_trail_timer{ 0.0 };
	bool m_is_hit_terrain{ false }; // 地面に当たったか
	static constexpr double TimeLimit = 8.0; // 8秒で消滅
	static constexpr double TrailInterval = 0.1; // 軌跡生成の間隔（秒）
	static constexpr float TurnRate = 2.0f; // ホーミング速度

public:
	EnemyBullet(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target_position, float speed)
		: m_position(position), m_speed(speed)
	{
		//方向ベクトルを初期化
		XMVECTOR dir = XMLoadFloat3(&target_position) - XMLoadFloat3(&position);
		dir = XMVector3Normalize(dir);
		XMStoreFloat3(&m_velocity, dir * speed);
	}

	void Update(double elapsed_time) {
		m_accumulated_time += elapsed_time;

		// ホーミング処理（プレイヤー方向へ）
		const XMFLOAT3& playerPos = Player::GetInstance().GetPosition();
		XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_position);
		toPlayer = XMVector3Normalize(toPlayer);

		XMVECTOR currentVel = XMLoadFloat3(&m_velocity);
		XMVECTOR currentDir = XMVector3Normalize(currentVel);

		// 徐々にプレイヤー方向へ補間
		XMVECTOR newDir = XMVectorLerp(currentDir, toPlayer, TurnRate * (float)elapsed_time);
		newDir = XMVector3Normalize(newDir);

		//速度を設定
		XMStoreFloat3(&m_velocity, newDir * m_speed);

		//位置更新
		XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * (float)elapsed_time);

		// 地形との衝突判定
		TerrainMesh* terrain = Map_GetTerrainMesh();
		if (terrain != nullptr) {
			// 球と地形の当たり判定
			float radius = 0.3f; // 半径
			SphereCollisionResult result = TerrainCollision_SphereVsTerrain(terrain, m_position, radius);
			if (result.hit) {
				m_is_hit_terrain = true;
				// 衝突時の小さなエフェクト
				for (int i = 0; i < 8; i++) {
					XMFLOAT3 offset = {
						(rand() % 100 - 50) * 0.01f,
						(rand() % 100 - 50) * 0.01f,
						(rand() % 100 - 50) * 0.01f
					};
					XMFLOAT3 effectPos;
					XMStoreFloat3(&effectPos, XMLoadFloat3(&m_position) + XMLoadFloat3(&offset));
					Trajectory3D_Create(effectPos, { 1.0f, 0.3f, 0.3f, 0.9f }, 0.4f, 0.8);
				}
				return;
			}
		}

		// 軌跡生成
		m_trail_timer += elapsed_time;
		while (m_trail_timer >= TrailInterval) {
			m_trail_timer -= TrailInterval;
			Trajectory3D_Create(m_position, { 1.0f, 0.3f, 0.3f, 0.8f }, 0.25f, 1.0);
		}
	}

	const XMFLOAT3& GetPosition() const {
		return m_position;
	}

	bool IsDestroy() const {
		return m_accumulated_time >= TimeLimit || m_is_hit_terrain;
	}

	// 新規追加: 有効期限切れ（寿命）で消えたかを判定
	bool IsExpired() const {
		return m_accumulated_time >= TimeLimit;
	}

	// 新規追加: 地形に当たったか
	bool IsHitTerrain() const {
		return m_is_hit_terrain;
	}
};

static constexpr int MAX_ENEMY_BULLET = 256;

static EnemyBullet* g_pEnemyBullets[MAX_ENEMY_BULLET]{};
static int g_EnemyBulletCount{ 0 };

static MODEL* g_pEnemyBulletModel{ nullptr };

void EnemyBullet_Initialize()
{
	g_pEnemyBulletModel = ModelLoad("resource/model/enebul.fbx", 0.35f, false);

	for (int i = 0; i < g_EnemyBulletCount; i++) {
		delete g_pEnemyBullets[i];
	}

	g_EnemyBulletCount = 0;
}

void EnemyBullet_Finalize()
{
	if (g_pEnemyBulletModel) {
		ModelRelease(g_pEnemyBulletModel);
		g_pEnemyBulletModel = nullptr;
	}

	for (int i = 0; i < g_EnemyBulletCount; i++) {
		delete g_pEnemyBullets[i];
	}

	g_EnemyBulletCount = 0;
}

void EnemyBullet_Update(double elapsed_time)
{
	for (int i = 0; i < g_EnemyBulletCount; i++) {
		g_pEnemyBullets[i]->Update(elapsed_time);
	}

	//破棄処理
	for (int i = g_EnemyBulletCount - 1; i >= 0; i--) {
		if (g_pEnemyBullets[i]->IsDestroy()) {
			// 寿命切れで消えた場合は爆発エフェクトを再生
			if (g_pEnemyBullets[i]->IsExpired() && !g_pEnemyBullets[i]->IsHitTerrain()) {
				BulletHitEffect_Create(g_pEnemyBullets[i]->GetPosition());
			}
			EnemyBullet_Destroy(i);
		}
	}
}

void EnemyBullet_Draw()
{
	for (int i = 0; i < g_EnemyBulletCount; i++) {
		XMVECTOR position = XMLoadFloat3(&g_pEnemyBullets[i]->GetPosition());
		XMMATRIX mtxWorld = XMMatrixTranslationFromVector(position);
		ModelDrawUnlit(g_pEnemyBulletModel, mtxWorld);


		// デバッグ用コリジョン表示
		Sphere sphere = EnemyBullet_GetSphere(i);
		DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 1.0f, 1.0f }); // 紫色

	}
}

void EnemyBullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& target_position, float speed)
{
	if (g_EnemyBulletCount >= MAX_ENEMY_BULLET) {
		return;
	}

	g_pEnemyBullets[g_EnemyBulletCount++] = new EnemyBullet(position, target_position, speed);
}

void EnemyBullet_Destroy(int index)
{
	delete g_pEnemyBullets[index];
	g_pEnemyBullets[index] = g_pEnemyBullets[g_EnemyBulletCount - 1];
	g_EnemyBulletCount--;
}

int EnemyBullet_GetBulletCount()
{
	return g_EnemyBulletCount;
}

Sphere EnemyBullet_GetSphere(int index)
{
	return { g_pEnemyBullets[index]->GetPosition(), 0.3f }; // 半径0.3
}

const DirectX::XMFLOAT3& EnemyBullet_GetPosition(int index)
{
	return g_pEnemyBullets[index]->GetPosition();
}

