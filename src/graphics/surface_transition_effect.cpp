// ----------------------------------------------------
// プレイヤー周囲泡エフェクト [surface_transition_effect.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2026-02-25
// Version: 1.0
// ----------------------------------------------------
#include "surface_transition_effect.h"
#include "trajectory3d.h"
#include "underwater_effect.h"
#include <DirectXMath.h>
#include <random>

using namespace DirectX;

static std::mt19937 s_RandomEngine;

// 潜航時の泡/水しぶきの生成数
static constexpr int BUBBLE_COUNT_PER_EDGE = 10;
static constexpr int SPLASH_COUNT = 0;

// エフェクトの持続時間
static double s_EffectTimer = 0.0;
static constexpr double EFFECT_DURATION = 3.0; // 3秒間エフェクト表示

// 水中での泡生成用タイマー
static double s_UnderwaterBubbleTimer = 0.0;
static constexpr double UNDERWATER_BUBBLE_INTERVAL = 0.15; // 0.15秒ごとに泡を生成
static constexpr int UNDERWATER_BUBBLES_PER_SPAWN = 3; // 1回の生成で出す泡の数

// 停止時の泡生成調整
static constexpr double STATIONARY_BUBBLE_INTERVAL = 0.5; // 停止時は0.5秒ごと（少なくする）
static constexpr int STATIONARY_BUBBLES_PER_SPAWN = 1; // 停止時は1個のみ
static constexpr float MOVEMENT_THRESHOLD = 0.5f; // 移動判定の速度閾値

void SurfaceTransitionEffect_Initialize()
{
	std::random_device rd;
	s_RandomEngine.seed(rd());
	s_EffectTimer = 0.0;
	s_UnderwaterBubbleTimer = 0.0;
}

void SurfaceTransitionEffect_Finalize()
{
}

void SurfaceTransitionEffect_Update(double elapsed_time)
{
	if (s_EffectTimer > 0.0) {
		s_EffectTimer -= elapsed_time;
		if (s_EffectTimer < 0.0) {
			s_EffectTimer = 0.0;
		}
	}
}

void SurfaceTransitionEffect_Draw()
{
}

// 潜るときの泡
static void CreateEdgeBubbles(const XMFLOAT3& centerPos, const XMFLOAT3& frontDir, bool isDiving)
{
	std::uniform_real_distribution<float> distAngle(0.0f, XM_2PI);
	std::uniform_real_distribution<float> distRadius(25.0f, 40.0f); // カメラからの距離（横幅を拡大）
	std::uniform_real_distribution<float> distHeight(-8.0f, 8.0f); // 上下範囲を拡大
	std::uniform_real_distribution<float> distSize(0.3f, 0.8f);
	std::uniform_real_distribution<float> distLife(0.8f, 1.5f);
	std::uniform_real_distribution<float> distForward(3.0f, 12.0f); // 前方距離
	
	XMFLOAT4 bubbleColor = isDiving 
		? XMFLOAT4{ 0.7f, 0.9f, 1.0f, 0.7f }  // 潜航時：青白い泡
		: XMFLOAT4{ 0.9f, 0.95f, 1.0f, 0.8f }; // 浮上時：白っぽい泡

	// 前方ベクトルを正規化
	XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&frontDir));
	// 右方向ベクトルを計算（前方 × 上方向）
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

	for (int i = 0; i < BUBBLE_COUNT_PER_EDGE; i++) {
		float angle = distAngle(s_RandomEngine);
		float radius = distRadius(s_RandomEngine);
		float height = distHeight(s_RandomEngine);
		float forwardDist = distForward(s_RandomEngine);
		
		// カメラの向きに沿って円周上に配置（画面全体に見える位置）
		XMVECTOR centerVec = XMLoadFloat3(&centerPos);
		XMVECTOR bubblePosVec = centerVec
			+ forward * forwardDist    // 前方に配置
			+ right * cosf(angle) * radius  // 左右に大きく円周配置
			+ up * height; // 上下に配置
		
		XMFLOAT3 bubblePos;
		XMStoreFloat3(&bubblePos, bubblePosVec);
		
		float size = distSize(s_RandomEngine);
		double life = distLife(s_RandomEngine);
		
		Trajectory3D_Create(bubblePos, bubbleColor, size, life);
	}
}

// 水しぶきの生成
static void CreateSplash(const XMFLOAT3& centerPos, const XMFLOAT3& frontDir, bool isDiving, int count)
{
	std::uniform_real_distribution<float> distX(-3.0f, 3.0f); // 左右範囲を拡大
	std::uniform_real_distribution<float> distY(-2.0f, 4.0f); // 上下範囲を拡大
	std::uniform_real_distribution<float> distZ(-3.0f, 3.0f); // 深さ方向の範囲を拡大
	std::uniform_real_distribution<float> distSize(0.5f, 1.0f);
	std::uniform_real_distribution<float> distLife(0.5, 1.0);
	std::uniform_real_distribution<float> distForward(2.0f, 6.0f); // 前方距離
	
	// 水しぶきの色（白っぽい）
	XMFLOAT4 splashColor = isDiving
		? XMFLOAT4{ 0.6f, 0.85f, 1.0f, 0.9f }  // 潜航時：青みがかった白
		: XMFLOAT4{ 0.95f, 0.98f, 1.0f, 0.95f }; // 浮上時：ほぼ白

	// 前方ベクトルを正規化
	XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&frontDir));
	// 右方向ベクトルを計算
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

	for (int i = 0; i < count; i++) {
		float forwardDist = distForward(s_RandomEngine);
		
		XMVECTOR centerVec = XMLoadFloat3(&centerPos);
		XMVECTOR splashPosVec = centerVec
			+ forward * forwardDist
			+ right * distX(s_RandomEngine)
			+ up * distY(s_RandomEngine);
		
		XMFLOAT3 splashPos;
		XMStoreFloat3(&splashPos, splashPosVec);
		
		float size = distSize(s_RandomEngine);
		double life = distLife(s_RandomEngine);
		
		Trajectory3D_Create(splashPos, splashColor, size, life);
	}
}

// 水中の泡
static void CreateUnderwaterBubbles(const XMFLOAT3& playerPos, const XMFLOAT3& frontDir, int bubbleCount)
{
	// カメラ視野の周辺（画面の端）に泡を配置
	std::uniform_real_distribution<float> distForward(8.0f, 17.0f);
	std::uniform_real_distribution<float> distSide(-10.0f, 10.0f);
	std::uniform_real_distribution<float> distHeight(-13.0f, 3.0f);
	std::uniform_real_distribution<float> distSize(0.2f, 0.5f);
	std::uniform_real_distribution<float> distLife(1.5, 3.0);
	
	// 水中の泡の色（薄い青白）
	XMFLOAT4 bubbleColor{ 0.75f, 0.92f, 1.0f, 0.6f };
	
	// 水面高度を取得
	float waterSurfaceY = UnderwaterEffect_GetWaterSurfaceY();
	
	// 前方ベクトルを正規化
	XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&frontDir));
	// 右方向ベクトルを計算（前方 × 上方向）
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));
	
	for (int i = 0; i < bubbleCount; i++) {
		float forwardDist = distForward(s_RandomEngine);
		float sideDist = distSide(s_RandomEngine);
		float heightOffset = distHeight(s_RandomEngine);
		
		// プレイヤー位置から前方向に配置
		XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);
		XMVECTOR bubblePosVec = playerPosVec 
			+ forward * forwardDist     // 前方に配置（遠い）
			+ right * sideDist   // 左右にランダムオフセット（広い）
			+ up * heightOffset;     // 上下にランダムオフセット
		
		XMFLOAT3 bubblePos;
		XMStoreFloat3(&bubblePos, bubblePosVec);
		
		// 泡の高度が水面より上にならないようにクリップ
		if (bubblePos.y > waterSurfaceY) {
			bubblePos.y = waterSurfaceY - 0.1f; // 水面より少し下に配置
		}
		
		float size = distSize(s_RandomEngine);
		double life = distLife(s_RandomEngine);
		
		Trajectory3D_Create(bubblePos, bubbleColor, size, life);
	}
}

void SurfaceTransitionEffect_TriggerSurface(const DirectX::XMFLOAT3& position)
{
	// 浮上エフェクト（水中→水上）
	// エフェクトは生成しない（潜航時のみエフェクトを表示）
	s_EffectTimer = 0.0;
	(void)position;

	// 画面端に泡を配置
	// CreateEdgeBubbles(position, false);
	
	// 中心付近に水しぶきを生成
	// CreateSplash(position, false, 10);
	
	// 追加の大きな泡を数個生成（視覚的なアクセント）
	// std::uniform_real_distribution<float> distOffset(-2.0f, 2.0f);
	// for (int i = 0; i < 5; i++) {
	// 	XMFLOAT3 bigBubblePos;
	// 	bigBubblePos.x = position.x + distOffset(s_RandomEngine);
	// 	bigBubblePos.y = position.y + distOffset(s_RandomEngine) * 0.5f;
	// 	bigBubblePos.z = position.z + distOffset(s_RandomEngine);
	// 	
	// 	Trajectory3D_Create(bigBubblePos, { 0.85f, 0.95f, 1.0f, 0.9f }, 1.5f, 1.8);
	// }
}

void SurfaceTransitionEffect_TriggerDive(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& frontDirection)
{
	// 潜航エフェクト（水上→水中）
	s_EffectTimer = EFFECT_DURATION;
	
	// 画面端に泡を配置（カメラの向き方向を考慮）
	CreateEdgeBubbles(position, frontDirection, true);
	
	// 中心付近に水しぶきを生成（カメラの向き方向を考慮）
	CreateSplash(position, frontDirection, true, 10);
	
	// 追加の泡トレイル（潜航の動きを強調）
	XMVECTOR forward = XMVector3Normalize(XMLoadFloat3(&frontDirection));
	std::uniform_real_distribution<float> distTrail(-1.5f, 1.5f);
	for (int i = 0; i < 8; i++) {
		XMVECTOR trailPosVec = XMLoadFloat3(&position) 
			+ forward * (2.0f + (float)i * 0.5f);  // 前方方向に並べる
		
		XMFLOAT3 trailPos;
		XMStoreFloat3(&trailPos, trailPosVec);
		trailPos.x += distTrail(s_RandomEngine);
		trailPos.z += distTrail(s_RandomEngine);
		
		Trajectory3D_Create(trailPos, { 0.65f, 0.88f, 1.0f, 0.8f }, 0.8f, 1.5);
	}
}

void SurfaceTransitionEffect_UpdateUnderwaterBubbles(
	double elapsed_time, 
	bool isUnderwater, 
	const DirectX::XMFLOAT3& position,
	const DirectX::XMFLOAT3& frontDirection,
	const DirectX::XMFLOAT3& velocity)
{
	if (!isUnderwater) {
		// 水中にいない場合はタイマーをリセット
		s_UnderwaterBubbleTimer = 0.0;
		return;
	}
	
	// プレイヤーの速度の大きさを計算
	XMVECTOR velVec = XMLoadFloat3(&velocity);
	float speed = XMVectorGetX(XMVector3Length(velVec));
	
	// 停止状態かどうかを判定
	bool isStationary = (speed < MOVEMENT_THRESHOLD);
	
	// 生成間隔と泡の数を決定
	double bubbleInterval = isStationary ? STATIONARY_BUBBLE_INTERVAL : UNDERWATER_BUBBLE_INTERVAL;
	int bubbleCount = isStationary ? STATIONARY_BUBBLES_PER_SPAWN : UNDERWATER_BUBBLES_PER_SPAWN;
	
	// 水中にいる場合、タイマーを進める
	s_UnderwaterBubbleTimer += elapsed_time;
	
	// 一定間隔で泡を生成
	while (s_UnderwaterBubbleTimer >= bubbleInterval) {
		s_UnderwaterBubbleTimer -= bubbleInterval;
		CreateUnderwaterBubbles(position, frontDirection, bubbleCount);
	}
}
