// ----------------------------------------------------
// 弾丸の管理 [bullet.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-12
// Version: 1.0
// ----------------------------------------------------
#include "bullet.h"
using namespace DirectX;
#include "model.h"
#include "trajectory3d.h"
#include "debug_draw.h"
#include "map.h"
#include "terrain_collision.h"

// 水面の高さ（ちょっと水面から顔を出すくらい）
static constexpr float WATER_SURFACE_Y = 0.3f;

class Bullet
{
private:
	XMFLOAT3 m_position{};
	XMFLOAT3 m_velocity{};
	double m_accumulated_time{ 0.0 };
	double m_trail_timer{ 0.0 }; // 軌跡のためのタイマー
	bool m_is_hit_terrain{ false }; // 地形に当たったフラグ
	bool m_is_intercept_assigned{ false }; // インターセプト対象として既に割り当て済みか
	static constexpr double TimeLImit = 14; // 14秒で消滅
	static constexpr double TrailInterval = 0.07; // 軌跡の間隔(秒)

public:
	Bullet(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
		: m_position(position), m_velocity(velocity), m_is_intercept_assigned(false)
	{
	}

	void Update(double elapsed_time) {
		m_accumulated_time += elapsed_time;
		
		// 位置を更新
		XMStoreFloat3(&m_position, XMLoadFloat3(&m_position) + XMLoadFloat3(&m_velocity) * static_cast<float>(elapsed_time));
		
		// 水面より上に出ようとした場合、水面に沿って進む
		if (m_position.y > WATER_SURFACE_Y) {
			m_position.y = WATER_SURFACE_Y; // Y座標を水面に固定
			
			// 速度のY成分が正（上向き）の場合は0にする
			if (m_velocity.y > 0.0f) {
				m_velocity.y = 0.0f;
			}
		}
		
		// 地形メッシュとの衝突判定
		TerrainMesh* terrain = Map_GetTerrainMesh();
		if (terrain != nullptr) {
			// 球体コライダーを使って地形との衝突判定
			float radius = 0.3f; // 弾丸の半径（適宜調整）
			SphereCollisionResult result = TerrainCollision_SphereVsTerrain(terrain, m_position, radius);
			if (result.hit) {
				m_is_hit_terrain = true;
				// ヒット時のエフェクト（爆発泡を生成）
				for (int i = 0; i < 8; i++) {
					XMFLOAT3 offset = {
						(rand() % 100 - 50) * 0.01f,
						(rand() % 100 - 50) * 0.01f,
						(rand() % 100 - 50) * 0.01f
					};
					XMFLOAT3 effectPos;
					XMStoreFloat3(&effectPos, XMLoadFloat3(&m_position) + XMLoadFloat3(&offset));
					Trajectory3D_Create(effectPos, { 1.0f, 0.8f, 0.3f, 0.9f }, 0.4f, 0.8);
				}
				return;
			}
		}
		
		// トレイル用タイマーを進め、間隔を超えたら航跡を生成する
		m_trail_timer += elapsed_time;
		while (m_trail_timer >= TrailInterval) {
			m_trail_timer -= TrailInterval;
			// メインの航跡
			Trajectory3D_Create(m_position, { 0.6f, 0.9f, 1.0f, 0.8f }, 0.3f, 1.5);
			// 少しだけ小さな航跡を追加（泡のゆらぎを出す)
			if (static_cast<int>(m_accumulated_time * 10) % 3 == 0) {
				Trajectory3D_Create(m_position, { 0.7f, 0.95f, 1.0f, 0.6f }, 0.2f, 1.2);
			}
		}
	}

	const XMFLOAT3& GetPosition() const {
		return m_position;
	}
	
	const XMFLOAT3& GetVelocity() const {
		return m_velocity;
	}

	XMFLOAT3 GetFront() const {
		XMFLOAT3 front;
		XMStoreFloat3(&front, XMVector3Normalize(XMLoadFloat3(&m_velocity)));
		return front;
	}

	bool IsDestroy() const {
		return m_accumulated_time >= TimeLImit || m_is_hit_terrain;
	}
	
	bool IsInterceptAssigned() const {
		return m_is_intercept_assigned;
	}
	
	void SetInterceptAssigned(bool assigned) {
		m_is_intercept_assigned = assigned;
	}
};

static constexpr int MAX_BULLET = 1024;

static Bullet* g_pBullets[MAX_BULLET]{};

static int g_BulletsCount{ 0 };

static MODEL* g_BulletModel{nullptr};

void Bullet_Initialize()
{
	g_BulletModel = ModelLoad("resource/model/bul.fbx", 0.25f, false);

	for (int i = 0; i < g_BulletsCount; i++) {
		delete g_pBullets[i];
	}

	g_BulletsCount = 0;
}

void Bullet_Finalize()
{
	ModelRelease(g_BulletModel);
	for (int i = 0; i < g_BulletsCount; i++) {
		delete g_pBullets[i];
	}

	g_BulletsCount = 0;
}

void Bullet_Update(double elapsed_time)
{
	for (int i = 0; i < g_BulletsCount; i++) {
		g_pBullets[i]->Update(elapsed_time);
	}

	for(int i = 0; i < g_BulletsCount; i++ ) {
		if (g_pBullets[i]->IsDestroy()) {
			Bullet_Destroy(i);
		}
		else {
			i++;
		}
	}
	for(int i = 0; i < g_BulletsCount; i++) {
		// 衝突判定
		g_pBullets[i]->Update(elapsed_time);
	}
}


void Bullet_Draw()
{
	// ライトの設定もここでする

	XMMATRIX mtxWorld{};

	for (int i = 0; i < g_BulletsCount; i++) {
		XMVECTOR position = XMLoadFloat3(&g_pBullets[i]->GetPosition());

		mtxWorld = XMMatrixTranslationFromVector(position);
		ModelDraw(g_BulletModel, mtxWorld);

#ifdef _DEBUG
		// デバッグビルド時に球のコライダーを描く
		Sphere sphere = Bullet_GetSphere(i);
		DebugDraw_Sphere(sphere, { 0.0f, 1.0f, 0.0f, 1.0f }); // 緑色
#endif // _DEBUG
	}
}

// シャドウマップ専用描画関数（深度のみ）
void Bullet_DrawShadow()
{
	XMMATRIX mtxWorld{};

	for (int i = 0; i < g_BulletsCount; i++) {
		XMVECTOR position = XMLoadFloat3(&g_pBullets[i]->GetPosition());
		mtxWorld = XMMatrixTranslationFromVector(position);
		ModelDrawShadow(g_BulletModel, mtxWorld);
	}
}

void Bullet_Create(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& velocity)
{
	if (g_BulletsCount >= MAX_BULLET) {
		return;
	}

	g_pBullets[g_BulletsCount] = new Bullet(position, velocity);
	g_BulletsCount++;
}

void Bullet_Destroy(int index)
{
	delete g_pBullets[index];
	g_pBullets[index] = g_pBullets[g_BulletsCount - 1];
	g_BulletsCount--;
}

int Bullet_GetBulletCount()
{
	return g_BulletsCount;
}

AABB Bullet_GetAABB(int index)
{
	// モデルのゲットAABBを返す
	return Model_GetAABB(g_BulletModel, g_pBullets[index]->GetPosition());
}

Sphere Bullet_GetSphere(int index)
{
	return {g_pBullets[index]->GetPosition(), g_BulletModel->local_aabb.GetHalfSize().x};
}

const DirectX::XMFLOAT3& Bullet_GetPosition(int index)
{
	return g_pBullets[index]->GetPosition();
}

const DirectX::XMFLOAT3& Bullet_GetVelocity(int index)
{
	return g_pBullets[index]->GetVelocity();
}

bool Bullet_IsInterceptAssigned(int index)
{
	return g_pBullets[index]->IsInterceptAssigned();
}

void Bullet_SetInterceptAssigned(int index, bool assigned)
{
	g_pBullets[index]->SetInterceptAssigned(assigned);
}
