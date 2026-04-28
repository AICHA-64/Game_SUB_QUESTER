// ----------------------------------------------------
// 通常の敵制御 [enemy_normal.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-26
// Version: 1.0
// ----------------------------------------------------
#include "enemy_normal.h"
#include "collision.h"
#include "player.h"
using namespace DirectX;
#include "cube.h"
#include "shader3d.h"
#include "model.h"
#include "debug_draw.h"
#include "Audio.h"
#include "trajectory3d.h"
#include "bullet_hit_effect.h"
#include "map.h"
#include "terrain_collision.h"

// 静的メンバの定義
MODEL* EnemyNormal::s_pModel = nullptr;
int EnemyNormal::s_PonSeId = -1;
int EnemyNormal::s_ExplosionSeId = -1;

const float PATROL_RANGE = 5.0f;  // パトロール範囲

// モデルリソースの初期化
void EnemyNormal_InitializeResource()
{
	if (EnemyNormal::s_pModel == nullptr) {
		EnemyNormal::s_pModel = ModelLoad("resource/model/ene.fbx", 1.0f, false);
	}
}

// モデルリソースの解放
void EnemyNormal_FinalizeResource()
{
	if (EnemyNormal::s_pModel != nullptr) {
		ModelRelease(EnemyNormal::s_pModel);
		EnemyNormal::s_pModel = nullptr;
	}
}

// オーディオリソースの初期化
void EnemyNormal_InitializeAudio()
{
	if (EnemyNormal::s_PonSeId == -1) {
		EnemyNormal::s_PonSeId = LoadAudio("resource/audio/pon.wav");
	}
	if (EnemyNormal::s_ExplosionSeId == -1) {
		EnemyNormal::s_ExplosionSeId = LoadAudio("resource/audio/miniex.wav");
	}
}

// オーディオリソースの解放
void EnemyNormal_FinalizeAudio()
{
	if (EnemyNormal::s_PonSeId != -1) {
		UnloadAudio(EnemyNormal::s_PonSeId);
		EnemyNormal::s_PonSeId = -1;
	}
	if (EnemyNormal::s_ExplosionSeId != -1) {
		UnloadAudio(EnemyNormal::s_ExplosionSeId);
		EnemyNormal::s_ExplosionSeId = -1;
	}
}

// コンストラクタの実装
EnemyNormal::EnemyNormal(const DirectX::XMFLOAT3& position) : m_Position(position), m_IsGuardMode(false), m_IsIntercepting(false)
{
	AdjustPositionToTerrain();
	
	// 初期状態はパトロール(ガードモードの場合は後でSetGuardMode呼び出しでガード状態に変更)
	ChangeState(new EnemyNormalStatePatrol(this));
}

// ガードモードを設定
void EnemyNormal::SetGuardMode(bool isGuard)
{
	m_IsGuardMode = isGuard;
	if (isGuard) {
		// ガードモードの場合はガード待機状態へ遷移
		ChangeState(new EnemyNormalStateGuardIdle(this));
	}
}

// インターセプト状態へ遷移
void EnemyNormal::StartIntercept(const DirectX::XMFLOAT3& bulletPos)
{
	if (m_IsGuardMode && !m_IsIntercepting) {
		m_IsIntercepting = true;
		ChangeState(new EnemyNormalStateIntercept(this, bulletPos));
	}
}


// シャドウマップ専用描画
void EnemyNormal::DrawShadow() const
{
	if (s_pModel != nullptr) {
		XMMATRIX mtxWorld = XMMatrixTranslation(m_Position.x, m_Position.y, m_Position.z);
		ModelDrawShadow(s_pModel, mtxWorld);
	}
}

// 地形に合わせて位置を調整する
void EnemyNormal::AdjustPositionToTerrain()
{
	TerrainMesh* terrain = Map_GetTerrainMesh();
	if (!terrain) {
#ifdef _DEBUG
		OutputDebugStringA("[EnemyNormal] WARNING: TerrainMesh is NULL during initialization!\n");
#endif
		return;
	}
	
	// 初期Y座標を保存
	float initialY = m_Position.y;
	
	// 地形の高さを取得
	float groundY = 0.0f;
	XMFLOAT3 checkPos = m_Position;
	checkPos.y = RAY_CAST_HEIGHT; // 上空から
	
	bool hasGround = TerrainCollision_GetGroundHeight(terrain, checkPos, groundY, nullptr);
	
	if (hasGround) {
		// 初期Y座標が地形より下の場合のみ、地形の少し上に配置
		if (initialY < groundY) {
			m_Position.y = groundY + GROUND_OFFSET_Y;
		} else {
			m_Position.y = initialY; // 初期Y座標を維持
		}
	} else {
		// 地形がない場合は初期Y座標を使用
		m_Position.y = initialY;
		return;
	}
	
	// 地形との衝突チェック（地形内部に埋まっていないか確認）
	for (int attempt = 0; attempt < MAX_ADJUST_ATTEMPTS; ++attempt) {
		SphereCollisionResult result = TerrainCollision_SphereVsTerrain(
			terrain,
			m_Position,
			COLLISION_RADIUS
		);
		
		if (result.hit) {
			// 地形に埋まっている場合は押し出す
			XMVECTOR pos = XMLoadFloat3(&m_Position);
			XMVECTOR push = XMLoadFloat3(&result.pushVector);
			pos = XMVectorAdd(pos, push);
			XMStoreFloat3(&m_Position, pos);
		} else {
			// 衝突がなければ完了
			break;
		}
	}
}

void EnemyNormal::EnemyNormalStatePatrol::Update(double elapsed_time)
{
	g_AccumulatedTime += elapsed_time;
	
	// クールダウンタイマーの更新
	if (m_TurnCooldown > 0.0f) {
		m_TurnCooldown -= (float)elapsed_time;
	}
	
	// 減速中でなければ加速
	if (!m_IsDecelerating) {
		m_CurrentSpeed += m_Acceleration * (float)elapsed_time;
		if (m_CurrentSpeed > m_MaxSpeed) {
			m_CurrentSpeed = m_MaxSpeed;
		}
	}
	
	// 移動量を計算
	float moveAmount = m_CurrentSpeed * m_MoveDirection * (float)elapsed_time;
	
	// 次の位置を予測
	XMFLOAT3 nextPos = m_pOwner->m_Position;
	nextPos.x += moveAmount;
	
	// 地形衝突判定（移動前にチェック）
	TerrainMesh* terrain = Map_GetTerrainMesh();
	bool terrainCollision = false;
	
	if (terrain && m_TurnCooldown <= 0.0f) {
		// 予測位置での衝突判定
		SphereCollisionResult result = TerrainCollision_SphereVsTerrain(
			terrain,
			nextPos,
			COLLISION_RADIUS // エネミーの半径
		);
		
		if (result.hit) {
			// 水平方向の押し出し成分をチェック（XZ平面）
			float horizontalPush = sqrtf(result.pushVector.x * result.pushVector.x + 
			        result.pushVector.z * result.pushVector.z);
			
			// 水平方向に押し出されている場合は地形衝突とみなす
			if (horizontalPush > HORIZONTAL_PUSH_THRESHOLD) {
				terrainCollision = true;
			}
		}
	}
	
	// 地形衝突時の処理
	if (terrainCollision) {
		// 減速開始
		m_IsDecelerating = true;
		m_CurrentSpeed -= m_Deceleration * (float)elapsed_time;
		
		// 完全に停止したら方向反転
		if (m_CurrentSpeed <= MIN_SPEED_THRESHOLD) {
			m_CurrentSpeed = 0.0f;
			m_MoveDirection = -m_MoveDirection;  // 方向反転
			m_IsDecelerating = false;            // 減速終了、加速開始
			m_TurnCooldown = TURN_COOLDOWN_TIME; // クールダウン開始
			
#ifdef _DEBUG
			char buf[128];
			sprintf_s(buf, "[EnemyNormal] Patrol: Reversed direction to %.1f at pos (%.2f, %.2f, %.2f)\n",
				m_MoveDirection, m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
			OutputDebugStringA(buf);
#endif
		}
		
		// 衝突中は移動しない
		moveAmount = 0.0f;
	} else {
		// 衝突していなければ減速状態を解除
		m_IsDecelerating = false;
	}
	
	// 実際に移動
	m_pOwner->m_Position.x += moveAmount;
	
	// パトロール範囲の制限（基準点から離れすぎたら反転）
	float distFromBase = m_pOwner->m_Position.x - m_PointX;
	
	if (fabsf(distFromBase) > PATROL_RANGE && m_TurnCooldown <= 0.0f) {
		// 範囲外に出たら減速開始
		if ((distFromBase > 0 && m_MoveDirection > 0) ||
		    (distFromBase < 0 && m_MoveDirection < 0)) {
			m_IsDecelerating = true;
			m_CurrentSpeed -= m_Deceleration * (float)elapsed_time;
			
			if (m_CurrentSpeed <= MIN_SPEED_THRESHOLD) {
				m_CurrentSpeed = 0.0f;
				m_MoveDirection = -m_MoveDirection;
				m_IsDecelerating = false;
				m_TurnCooldown = TURN_COOLDOWN_TIME;
			}
		}
	}
	
	// Y座標の調整（地形の高さに追従、ただし上昇は制限）
	if (terrain) {
		float groundY = 0.0f;
		XMFLOAT3 checkPos = m_pOwner->m_Position;
		checkPos.y = 100.0f; // 上空から
		
		if (TerrainCollision_GetGroundHeight(terrain, checkPos, groundY, nullptr)) {
			// 現在のY座標
			float currentY = m_pOwner->m_Position.y;
			float targetY = groundY + 0.5f;
			
			// 水面の閾値（game.cppと同じ値）
			const float WATER_SURFACE_Y = -1.0f;
			
			// 水面より上には出ないように制限
			if (targetY > WATER_SURFACE_Y) {
				targetY = WATER_SURFACE_Y;
			}
			
			// 下方向への移動は許可、上方向への移動は緩やかに
			if (targetY < currentY) {
				// 下がる場合は即座に追従
				m_pOwner->m_Position.y = targetY;
			} else {
				// 上がる場合は緩やかに（じわじわ登らないように）
				float maxRise = 0.5f * (float)elapsed_time;  // 1秒で0.5までしか上昇しない
				float rise = targetY - currentY;
				if (rise > maxRise) {
					rise = maxRise;
				}
				m_pOwner->m_Position.y = currentY + rise;
			}
		}
	}

	// プレイヤー検知 → チェイス状態へ
	if(Collision_IsOverlapSphere(
		{ m_pOwner->m_Position, m_pOwner->m_DetectionRadius },
		Player::GetInstance().GetPosition())) 
	{
		m_pOwner->ChangeState(new EnemyNormalStateChase(m_pOwner));
	}
}

void EnemyNormal::EnemyNormalStatePatrol::Draw() const
{
	// FBXモデルで描画
	if (EnemyNormal::s_pModel != nullptr) {
		XMMATRIX mtxWorld = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		ModelDraw(EnemyNormal::s_pModel, mtxWorld);
	}

	// デバッグビルド時に球体コライダーを可視化
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
}

void EnemyNormal::EnemyNormalStateChase::Update(double elapsed_time)
{
	m_ChaseTime += elapsed_time;
	// 探索間隔に揺らぎを持たせて更新
	if (m_ChaseTime > m_SearchInterval) {
		m_ChaseTime = 0.0;
		// 経路探索を実行
		m_RouteData = RouteSearch_Search(
			m_pOwner->m_Position.x, m_pOwner->m_Position.z,
			Player::GetInstance().GetPosition().x, Player::GetInstance().GetPosition().z);
		
		// 次のターゲットインデックスを設定
		if (m_RouteData.count > 1) {
			m_TargetRouteIndex = 1;
		} else {
			m_TargetRouteIndex = 0;
		}

		// 探索間隔をランダム化して同期を防ぐ (±0.5秒)
		double jitter = ((double)std::rand() / (double)RAND_MAX - 0.5) * 1.0; // [-0.5,0.5]
		m_SearchInterval = 2.0 + jitter;
	}

	// ルートが見つからない場合はプレイヤーに直進
	if (m_RouteData.count <= 0) {
		XMVECTOR toPlayer = XMLoadFloat3(&Player::GetInstance().GetPosition()) - XMLoadFloat3(&m_pOwner->m_Position);
		float distToPlayer = XMVectorGetX(XMVector3Length(toPlayer));
		if (distToPlayer > 0.1f) {
			toPlayer = XMVector3Normalize(toPlayer);
			XMVECTOR position = XMLoadFloat3(&m_pOwner->m_Position) + toPlayer * 1.0f * (float)elapsed_time;
			XMStoreFloat3(&m_pOwner->m_Position, position);
		}
	}
	else {
		// 計算されたルートを使用
		if (m_TargetRouteIndex < 0) m_TargetRouteIndex = 0;
		if (m_TargetRouteIndex >= m_RouteData.count) m_TargetRouteIndex = m_RouteData.count - 1;

		int routeIndex = 0;
		if (m_RouteData.count > 0 && m_TargetRouteIndex >= 0 && m_TargetRouteIndex < m_RouteData.count) {
			routeIndex = m_RouteData.route[m_TargetRouteIndex];
		} else {
			routeIndex = 0;
		}

		XMVECTOR targetPos = XMVectorSet(
			RouteSearch_GetXByIndex(routeIndex),
			RouteSearch_GetYByIndex(routeIndex) + 0.5f,
			RouteSearch_GetZByIndex(routeIndex),
			0.0f);

		XMVECTOR position = XMLoadFloat3(&m_pOwner->m_Position);
		XMVECTOR toTarget = targetPos - position;
		float distToTarget = XMVectorGetX(XMVector3Length(toTarget));
		if (distToTarget > 0.1f) {
			XMVECTOR dir = XMVector3Normalize(toTarget);
			float moveSpeed = 4.0f;
			float moveDistance = moveSpeed * (float)elapsed_time;
			if (moveDistance > distToTarget) moveDistance = distToTarget;
			position = position + dir * moveDistance;
			XMStoreFloat3(&m_pOwner->m_Position, position);
		}

		if (distToTarget < 0.3f) {
			if (m_TargetRouteIndex < m_RouteData.count - 1) {
				m_TargetRouteIndex++;
			} else {
				m_ChaseTime = 2.0; // すぐに次の経路探索を行うようにスケジュール
			}
		}
	}

	// 地形との衝突判定
	TerrainMesh* terrain = Map_GetTerrainMesh();
	if (terrain) {
		SphereCollisionResult result = TerrainCollision_SphereVsTerrain(
			terrain,
			m_pOwner->m_Position,
			COLLISION_RADIUS
		);
		if (result.hit) {
			XMVECTOR pos = XMLoadFloat3(&m_pOwner->m_Position);
			XMVECTOR push = XMLoadFloat3(&result.pushVector);
			pos = pos + push;
			XMStoreFloat3(&m_pOwner->m_Position, pos);
		}
	}

	// 爆発状態への遷移チェック
	XMVECTOR toPlayer = XMLoadFloat3(&Player::GetInstance().GetPosition()) - XMLoadFloat3(&m_pOwner->m_Position);
	float distToPlayer = XMVectorGetX(XMVector3Length(toPlayer));
	if (distToPlayer <= m_pOwner->m_ExplosionRadius) {
		m_pOwner->ChangeState(new EnemyNormalStateExplosion(m_pOwner));
		return;
	}

	// プレイヤーを見失った場合、一定時間後にパトロールへ戻る
	if (!Collision_IsOverlapSphere({ m_pOwner->m_Position, m_pOwner->m_DetectionRadius }, Player::GetInstance().GetPosition())) {
		m_AccumulatedTime += elapsed_time;
		if (m_AccumulatedTime >= 3.0) {
			m_pOwner->ChangeState(new EnemyNormalStatePatrol(m_pOwner));
		}
	} else {
		m_AccumulatedTime = 0.0;
	}
}

void EnemyNormal::EnemyNormalStateChase::Draw() const
{
	if (EnemyNormal::s_pModel != nullptr) {
		XMMATRIX mtxWorld = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		ModelDraw(EnemyNormal::s_pModel, mtxWorld);
	}

	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
}

// 爆発ステートの更新
void EnemyNormal::EnemyNormalStateExplosion::Update(double elapsed_time)
{
	// 初回フレームで最初のSEを再生
	if (m_LastPlayedSecond == 3) {
		if (s_PonSeId != -1) {
			PlayAudio(s_PonSeId, false);
		}
		m_LastPlayedSecond = 2; // 次は2秒時点で再生
	}
	
	m_CountdownTime -= elapsed_time;
	
	// 1秒ごとにSEを再生
	int currentSecond = (int)m_CountdownTime;
	if (currentSecond < m_LastPlayedSecond && currentSecond >= 0) {
		m_LastPlayedSecond = currentSecond;
		if (s_PonSeId != -1) {
			PlayAudio(s_PonSeId, false);
		}
	}
	
	// カウントダウン終了時に爆発
	if (m_CountdownTime <= 0.0 && !m_HasExploded) {
		m_HasExploded = true;
		
		// 爆発音を再生
		if (s_ExplosionSeId != -1) {
			PlayAudio(s_ExplosionSeId, false);
		}
		
		// 爆発エフェクトを生成
		for (int i = 0; i < 6; i++) {
			XMFLOAT3 offset = {
				(rand() % 100 - 50) * 0.02f,
				(rand() % 100 - 50) * 0.02f,
				(rand() % 100 - 50) * 0.02f
			};
			XMFLOAT3 effectPos;
			XMStoreFloat3(&effectPos, XMLoadFloat3(&m_pOwner->m_Position) + XMLoadFloat3(&offset));
			// 爆発エフェクト
			XMFLOAT4 color = { 1.0f, 0.5f + (rand() % 50) * 0.01f, 0.2f, 0.9f };
			BulletHitEffect_Create(effectPos);
		}
		
		// プレイヤーが爆発範囲内にいるかチェック
		XMVECTOR toPlayer = XMLoadFloat3(&Player::GetInstance().GetPosition()) - XMLoadFloat3(&m_pOwner->m_Position);
		float distToPlayer = XMVectorGetX(XMVector3Length(toPlayer));
		if (distToPlayer <= m_pOwner->m_ExplosionRadius) {
			// プレイヤーに1ダメージ
			Player::GetInstance().Damage(1);
		}
		
		// HPを0に
		m_pOwner->m_Hp = 0;
	}
}

// 爆発ステートの描画
void EnemyNormal::EnemyNormalStateExplosion::Draw() const
{
	// カウントダウン中は点滅
	if (EnemyNormal::s_pModel != nullptr) {
		// 点滅効果:0.2秒ごとに切り替え
		bool shouldDraw = ((int)(m_CountdownTime * 5) % 2) == 0;
		if (shouldDraw || m_HasExploded) {
			XMMATRIX mtxWorld = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
			ModelDraw(EnemyNormal::s_pModel, mtxWorld);
		}
	}

	// デバッグビルド時に爆発範囲を描画
#ifdef _DEBUG
	Sphere explosionSphere = { m_pOwner->m_Position, m_pOwner->m_ExplosionRadius };
	DebugDraw_Sphere(explosionSphere, { 1.0f, 1.0f, 0.0f, 0.5f }); // 黄色(爆発予告)
	
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
#endif
}

// =======================================================
// ガード待機状態(ラスボス周囲で散らばって待機)
// =======================================================

// ラスボスの位置を取得するヘルパー関数
static DirectX::XMFLOAT3 GetFinalBossPosition()
{
	// Enemy.cppからラスボスを探す
	extern int Enemy_GetEnemyCount();
	extern Enemy* Enemy_GetEnemy(int index);
	
	for (int i = 0; i < Enemy_GetEnemyCount(); i++) {
		Enemy* enemy = Enemy_GetEnemy(i);
		if (enemy && enemy->IsFinalBoss()) {
			return enemy->GetPosition();
		}
	}
	// ラスボスが見つからない場合は原点を返す
	return { 0.0f, 0.0f, 0.0f };
}

EnemyNormal::EnemyNormalStateGuardIdle::EnemyNormalStateGuardIdle(EnemyNormal* pOwner)
	: m_pOwner(pOwner), m_AccumulatedTime(0.0)
{
	// ラスボスの位置を取得
	XMFLOAT3 bossPos = GetFinalBossPosition();
	
	// 現在位置を巡回の中心とするが、ボスに近づける
	m_PatrolCenter = pOwner->m_Position;
	
	// ボスからの距離を計算
	float dx = m_PatrolCenter.x - bossPos.x;
	float dz = m_PatrolCenter.z - bossPos.z;
	float distFromBoss = sqrtf(dx * dx + dz * dz);
	
	// ボスからの最大距離を制限（15m以内に収める）
	constexpr float MAX_GUARD_DISTANCE = 15.0f;
	if (distFromBoss > MAX_GUARD_DISTANCE) {
		// ボス方向に引き寄せる
		float ratio = MAX_GUARD_DISTANCE / distFromBoss;
		m_PatrolCenter.x = bossPos.x + dx * ratio;
		m_PatrolCenter.z = bossPos.z + dz * ratio;
	}
	
	// ランダムな初期角度
	m_PatrolAngle = (float)rand() / (float)RAND_MAX * 2.0f * 3.14159265f;
	// 巡回半径を小さく（1?3m）
	m_PatrolRadius = 1.0f + (float)rand() / (float)RAND_MAX * 2.0f;
	// 巡回速度（0.3?0.8）
	m_PatrolSpeed = 0.3f + (float)rand() / (float)RAND_MAX * 0.5f;
}

void EnemyNormal::EnemyNormalStateGuardIdle::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;
	
	// ラスボスの位置を取得して、その周囲を巡回
	XMFLOAT3 bossPos = GetFinalBossPosition();
	
	// 巡回角度を更新
	m_PatrolAngle += m_PatrolSpeed * (float)elapsed_time;
	if (m_PatrolAngle > 2.0f * 3.14159265f) {
		m_PatrolAngle -= 2.0f * 3.14159265f;
	}
	
	// 巡回位置を計算(ボス周囲を円運動)
	// 各敵はスポーン時の位置を基準にした角度で巡回
	float baseAngle = atan2f(m_PatrolCenter.z - bossPos.z, m_PatrolCenter.x - bossPos.x);
	float currentAngle = baseAngle + sinf(m_PatrolAngle) * 0.3f; // ±0.3ラジアンの揺れ（小さく）
	
	// ボスからの距離を計算
	float dx = m_PatrolCenter.x - bossPos.x;
	float dz = m_PatrolCenter.z - bossPos.z;
	float distFromBoss = sqrtf(dx * dx + dz * dz);
	
	// 目標位置(ボスから一定距離で揺れる)
	XMFLOAT3 targetPos;
	targetPos.x = bossPos.x + cosf(currentAngle) * distFromBoss;
	targetPos.y = m_PatrolCenter.y + sinf(m_PatrolAngle * 2.0f) * 0.5f; // Y方向の揺れも小さく
	targetPos.z = bossPos.z + sinf(currentAngle) * distFromBoss;
	
	// 目標位置へゆっくり移動
	XMVECTOR vCurrent = XMLoadFloat3(&m_pOwner->m_Position);
	XMVECTOR vTarget = XMLoadFloat3(&targetPos);
	XMVECTOR toTarget = vTarget - vCurrent;
	float distToTarget = XMVectorGetX(XMVector3Length(toTarget));
	
	if (distToTarget > 0.1f) {
		toTarget = XMVector3Normalize(toTarget);
		float moveSpeed = 2.0f; // ゆっくり移動
		float moveAmount = moveSpeed * (float)elapsed_time;
		if (moveAmount > distToTarget) {
			moveAmount = distToTarget;
		}
		XMVECTOR newPos = vCurrent + toTarget * moveAmount;
		XMStoreFloat3(&m_pOwner->m_Position, newPos);
	}
	
	// 地形との衝突判定
	TerrainMesh* terrain = Map_GetTerrainMesh();
	if (terrain) {
		SphereCollisionResult result = TerrainCollision_SphereVsTerrain(
			terrain,
			m_pOwner->m_Position,
			COLLISION_RADIUS
		);
		if (result.hit) {
			XMVECTOR pos = XMLoadFloat3(&m_pOwner->m_Position);
			XMVECTOR push = XMLoadFloat3(&result.pushVector);
			pos = pos + push;
			XMStoreFloat3(&m_pOwner->m_Position, pos);
		}
	}
	
	// プレイヤーに近づきすぎたら爆発状態へ遷移
	XMFLOAT3 playerPos = Player::GetInstance().GetPosition();
	XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_pOwner->m_Position);
	float distToPlayer = XMVectorGetX(XMVector3Length(toPlayer));
	if (distToPlayer <= m_pOwner->m_ExplosionRadius) {
		m_pOwner->ChangeState(new EnemyNormalStateExplosion(m_pOwner));
		return;
	}
}

void EnemyNormal::EnemyNormalStateGuardIdle::Draw() const
{
	if (EnemyNormal::s_pModel != nullptr) {
		XMMATRIX mtxWorld = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		ModelDraw(EnemyNormal::s_pModel, mtxWorld);
	}

	// ガード待機状態は緑色で表示
	Sphere sphere = m_pOwner->GetCollision();
	DebugDraw_Sphere(sphere, { 0.0f, 0.8f, 0.2f, 1.0f }); // 緑色(待機中)
}

// =======================================================
// インターセプト状態(弾丸に当たりに行く)
// =======================================================

void EnemyNormal::EnemyNormalStateIntercept::Update(double elapsed_time)
{
	m_AccumulatedTime += elapsed_time;
	m_TimeoutTimer -= elapsed_time;
	
	// タイムアウトしたらガード待機状態に戻る
	if (m_TimeoutTimer <= 0.0) {
		m_pOwner->m_IsIntercepting = false;
		m_pOwner->ChangeState(new EnemyNormalStateGuardIdle(m_pOwner));
		return;
	}
	
	// 目標位置に到達済みの場合は、その場に留まる
	if (m_HasReachedTarget) {
		m_StayTimer -= elapsed_time;
		
		// 滞在時間が終わったらガード待機状態に戻る
		if (m_StayTimer <= 0.0) {
			m_pOwner->m_IsIntercepting = false;
			m_pOwner->ChangeState(new EnemyNormalStateGuardIdle(m_pOwner));
			return;
		}
		
		// 到達後は移動せず、その場で待機（弾丸との衝突を待つ）
		// ただし、プレイヤー接近チェックは行う
	} else {
		// ターゲット位置へ高速移動
		XMVECTOR vCurrent = XMLoadFloat3(&m_pOwner->m_Position);
		XMVECTOR vTarget = XMLoadFloat3(&m_TargetPosition);
		XMVECTOR toTarget = vTarget - vCurrent;
		float distToTarget = XMVectorGetX(XMVector3Length(toTarget));
		
		if (distToTarget > 1.0f) {
			toTarget = XMVector3Normalize(toTarget);
			float moveAmount = m_MoveSpeed * (float)elapsed_time;
			if (moveAmount > distToTarget) {
				moveAmount = distToTarget;
			}
			XMVECTOR newPos = vCurrent + toTarget * moveAmount;
			XMStoreFloat3(&m_pOwner->m_Position, newPos);
		} else {
			// 目標位置に到達
			m_HasReachedTarget = true;
			m_StayTimer = 2.0; // 2秒間その場に留まる
			
#ifdef _DEBUG
			char buf[128];
			sprintf_s(buf, "[EnemyNormal] Intercept: Reached target position, staying for %.1f seconds\n", m_StayTimer);
			OutputDebugStringA(buf);
#endif
		}
	}
	
	// 地形との衝突判定
	TerrainMesh* terrain = Map_GetTerrainMesh();
	if (terrain) {
		SphereCollisionResult result = TerrainCollision_SphereVsTerrain(
			terrain,
			m_pOwner->m_Position,
			COLLISION_RADIUS
		);
		if (result.hit) {
			XMVECTOR pos = XMLoadFloat3(&m_pOwner->m_Position);
			XMVECTOR push = XMLoadFloat3(&result.pushVector);
			pos = pos + push;
			XMStoreFloat3(&m_pOwner->m_Position, pos);
		}
	}
	
	// プレイヤーに近づきすぎたら爆発状態へ遷移
	XMFLOAT3 playerPos = Player::GetInstance().GetPosition();
	XMVECTOR toPlayer = XMLoadFloat3(&playerPos) - XMLoadFloat3(&m_pOwner->m_Position);
	float distToPlayer = XMVectorGetX(XMVector3Length(toPlayer));
	if (distToPlayer <= m_pOwner->m_ExplosionRadius) {
		m_pOwner->m_IsIntercepting = false;
		m_pOwner->ChangeState(new EnemyNormalStateExplosion(m_pOwner));
		return;
	}
}

void EnemyNormal::EnemyNormalStateIntercept::Draw() const
{
	if (EnemyNormal::s_pModel != nullptr) {
		XMMATRIX mtxWorld = XMMatrixTranslation(m_pOwner->m_Position.x, m_pOwner->m_Position.y, m_pOwner->m_Position.z);
		ModelDraw(EnemyNormal::s_pModel, mtxWorld);
	}

	// インターセプト状態はオレンジ色で表示
	Sphere sphere = m_pOwner->GetCollision();
	
	// 到達済みなら明るいオレンジ、移動中は暗いオレンジ
	if (m_HasReachedTarget) {
		DebugDraw_Sphere(sphere, { 1.0f, 0.6f, 0.0f, 1.0f }); // 明るいオレンジ(待機中)
	} else {
		DebugDraw_Sphere(sphere, { 1.0f, 0.3f, 0.0f, 1.0f }); // 暗いオレンジ(移動中)
	}
	
#ifdef _DEBUG
	// ターゲット位置を表示
	Sphere targetSphere = { m_TargetPosition, 0.5f };
	DebugDraw_Sphere(targetSphere, { 1.0f, 1.0f, 0.0f, 0.5f }); // 黄色(ターゲット)
#endif
}

