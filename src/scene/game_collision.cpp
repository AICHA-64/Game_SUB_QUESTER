// ----------------------------------------------------
// ゲームシーン 当たり判定処理分割 [game_collision.cpp]
// ====================================================
// 動作を変えないためにgame.cppからそのまま抽出しています
// ----------------------------------------------------
#include "game.h"
#include "collision.h"
#include "map.h"
#include "bullet.h"
#include "bullet_hit_effect.h"
#include "Enemy.h"
#include "enemy_bullet.h"
#include "enemy_normal.h"
#include "player.h"
#include "player_camera.h"
#include "player_upgrade.h"
#include "Audio.h"
#include <DirectXMath.h>
#include <cmath>
#include <cfloat>

using namespace DirectX;

void GameScene::CheckBulletMapCollision()
{
	// 弾とオブジェクトの当たり判定(Sphere vs AABB)
	for (int j = 0; j < Map_GetObjectsCount(); j++){
		for (int i = 0; i < Bullet_GetBulletCount(); i++) {
			Sphere bullet = Bullet_GetSphere(i);
			AABB object = Map_GetObject(j)->Aabb;
			if(Collision_IsOverlapSphereAABB(bullet, object)) {
				// 衝突したときの処理
				BulletHitEffect_Create(Bullet_GetPosition(i));
				Bullet_Destroy(i);
			}
		}
	}
}

void GameScene::CheckBulletEnemyCollision()
{
	// エネミーと弾の当たり判定(Sphere vs Sphere)
	for (int j = 0; j < Enemy_GetEnemyCount(); j++) {
		for (int i = 0; i < Bullet_GetBulletCount(); i++) {
			Sphere bullet = Bullet_GetSphere(i);
			Sphere enemy = Enemy_GetEnemy(j)->GetCollision();
			if (Collision_IsOverlapSphere(bullet, enemy)) {
				// 衝突したときの処理
				BulletHitEffect_Create(Bullet_GetPosition(i));
				Bullet_Destroy(i);
				
				// ボスかどうかでシェイクの強さを変える
				bool isBoss = Enemy_GetEnemy(j)->IsBoss();
				
				// ダメージに強化値を適用
				int baseDamage = 100;
				int damage = static_cast<int>(baseDamage * PlayerUpgrade_GetDamageMultiplier());
				Enemy_GetEnemy(j)->Damage(damage);
				
				// ボス撃破(ダメージで破壊されたか)
				if (isBoss && Enemy_GetEnemy(j)->IsDestroy()) {
					// ボス撃破時は大きなシェイク
					PlayerCamera_Shake_position(0.8f);
				} else {
					// 通常のシェイク
					PlayerCamera_Shake_position(0.2f);
				}
				
				PlayAudio(m_minExSeId, false);
				break; // 1フレーム1回だけダメージ
			}
		}
	}
}

void GameScene::CheckFinalBossIntercept()
{
	// ラスボス戦モード:自機弾がラスボスに向かっている場合、ガード敵が割り込む
	if (Enemy_IsFinalBossMode()) {
		// ラスボスの位置を取得
		XMFLOAT3 finalBossPos = { 0.0f, 0.0f, 0.0f };
		float bossCollisionRadius = 6.0f; // ラスボスのコリジョン半径
		for (int i = 0; i < Enemy_GetEnemyCount(); i++) {
			if (Enemy_GetEnemy(i)->IsFinalBoss()) {
				finalBossPos = Enemy_GetEnemy(i)->GetPosition();
				break;
			}
		}
		
		// 各自機弾に対してチェック
		for (int i = 0; i < Bullet_GetBulletCount(); i++) {
			// 既に迎撃対象として割り当て済みの弾はスキップ
			if (Bullet_IsInterceptAssigned(i)) continue;
			
			XMFLOAT3 bulletPos = Bullet_GetPosition(i);
			XMFLOAT3 bulletVel = Bullet_GetVelocity(i);
			
			// 弾の速度がない場合はスキップ
			float bulletSpeed = sqrtf(bulletVel.x * bulletVel.x + bulletVel.y * bulletVel.y + bulletVel.z * bulletVel.z);
			if (bulletSpeed < 0.1f) continue;
			
			// 弾の進行方向を正規化
			XMFLOAT3 bulletDir = {
				bulletVel.x / bulletSpeed,
				bulletVel.y / bulletSpeed,
				bulletVel.z / bulletSpeed
			};
			
			// 弾からラスボスへのベクトル
			XMFLOAT3 toBoss = {
				finalBossPos.x - bulletPos.x,
				finalBossPos.y - bulletPos.y,
				finalBossPos.z - bulletPos.z
			};
			float distToBoss = sqrtf(toBoss.x * toBoss.x + toBoss.y * toBoss.y + toBoss.z * toBoss.z);
			
			// ラスボスまでの距離が遠い場合はスキップ（迎撃の必要なし）
			if (distToBoss > 80.0f) continue;
			
			// 弾の進行方向とラスボスへの方向の内積を計算
			// 正なら弾はラスボスに向かっている
			float dotProduct = bulletDir.x * toBoss.x + bulletDir.y * toBoss.y + bulletDir.z * toBoss.z;
			
			// ラスボスに向かっていない弾はスキップ
			if (dotProduct < 0.0f) continue;
			
			// 弾の軌道とラスボスの最短距離を計算
			// 射影長 = dot(toBoss, bulletDir)
			float projectionLength = dotProduct; // bulletDirは正規化済み
			
			// 最接近点
			XMFLOAT3 closestPoint = {
				bulletPos.x + bulletDir.x * projectionLength,
				bulletPos.y + bulletDir.y * projectionLength,
				bulletPos.z + bulletDir.z * projectionLength
			};
			
			// 最短距離
			float closestDist = sqrtf(
				(closestPoint.x - finalBossPos.x) * (closestPoint.x - finalBossPos.x) +
				(closestPoint.y - finalBossPos.y) * (closestPoint.y - finalBossPos.y) +
				(closestPoint.z - finalBossPos.z) * (closestPoint.z - finalBossPos.z)
			);
			
			// 自機弾がラスボスに当たる軌道でない場合はスキップ
			// （距離がラスボスのコリジョン半径+余裕分より大きい）
			if (closestDist > bossCollisionRadius + 5.0f) continue;
			
			// 自機弾が現在位置からラスボスまでの距離のどこにいるか確認
			// 既にラスボスを通り過ぎている場合はスキップ
			if (projectionLength > distToBoss + bossCollisionRadius) continue;
			
			// 迎撃地点を計算（自機弾の現在位置とラスボスの間）
			// ラスボスの少し手前で防ぐ
			float interceptDistance = projectionLength * 0.7f; // 距離の70%地点
			if (interceptDistance < 5.0f) interceptDistance = 5.0f; // 最低5m先
			
			XMFLOAT3 interceptPoint = {
				bulletPos.x + bulletDir.x * interceptDistance,
				bulletPos.y + bulletDir.y * interceptDistance,
				bulletPos.z + bulletDir.z * interceptDistance
			};
			
			// 最も近いガード敵を探して迎撃（1体のみ）
			float closestGuardDist = FLT_MAX;
			EnemyNormal* closestGuard = nullptr;
			
			for (int j = 0; j < Enemy_GetEnemyCount(); j++) {
				EnemyNormal* normal = dynamic_cast<EnemyNormal*>(Enemy_GetEnemy(j));
				if (normal && normal->IsGuardMode() && !normal->IsIntercepting()) {
					// このガード敵と迎撃地点の距離を計算
					XMFLOAT3 guardPos = normal->GetPosition();
					float gdx = guardPos.x - interceptPoint.x;
					float kdy = guardPos.y - interceptPoint.y;
					float gdz = guardPos.z - interceptPoint.z;
					float guardDist = sqrtf(gdx * gdx + kdy * kdy + gdz * gdz);
					
					// 迎撃地点に間に合うか確認（ざっくり推測）
					float timeToIntercept = interceptDistance / bulletSpeed;
					float guardSpeed = 15.0f; // ガード敵の移動速度
					float guardTimeNeeded = guardDist / guardSpeed;
					
					// 間に合いそうなガード敵のみ対象にする
					if (guardTimeNeeded < timeToIntercept * 1.5f && guardDist < closestGuardDist) {
						closestGuardDist = guardDist;
						closestGuard = normal;
					}
				}
			}
			
			// 最も近いガード敵を迎撃に向かわせる（1体のみ）
			if (closestGuard != nullptr) {
				closestGuard->StartIntercept(interceptPoint);
				// この弾を割り当て済みとしてマーク
				Bullet_SetInterceptAssigned(i, true);
			}
		}
	}
}

void GameScene::CheckBulletVsEnemyBullet()
{
	// プレイヤーの弾とエネミー弾の当たり判定 (Sphere vs Sphere)
	// ループを逆回しで弾のインデックスに対応
	for (int ib = Bullet_GetBulletCount() - 1; ib >= 0; ib--) {
		Sphere pbullet = Bullet_GetSphere(ib);
		for (int jb = EnemyBullet_GetBulletCount() - 1; jb >= 0; jb--) {
			Sphere ebullet = EnemyBullet_GetSphere(jb);
			if (Collision_IsOverlapSphere(pbullet, ebullet)) {
				// 衝突エフェクト
				BulletHitEffect_Create(Bullet_GetPosition(ib));
				BulletHitEffect_Create(EnemyBullet_GetPosition(jb));
				// 両弾消滅
				Bullet_Destroy(ib);
				EnemyBullet_Destroy(jb);
				PlayAudio(m_minExSeId, false);
				// 弾同士の相殺時にカメラシェイク(弱め)
				PlayerCamera_Shake_position(0.15f);
				break; // このプレイヤー弾は消滅したので次の弾へ
			}
		}
	}
}

void GameScene::CheckEnemyBulletVsPlayer()
{
	// エネミー弾とプレイヤーの当たり判定(Sphere vs Sphere)
	if (m_DamageInvincibleTimer <= 0.0) {
		Sphere playerSphere = Player::GetInstance().GetSphere();
		for (int i = 0; i < EnemyBullet_GetBulletCount(); i++) {
			Sphere enemyBullet = EnemyBullet_GetSphere(i);
			if (Collision_IsOverlapSphere(playerSphere, enemyBullet)) {
				// プレイヤーにダメージ
				Player::GetInstance().Damage(1);
				BulletHitEffect_Create(EnemyBullet_GetPosition(i));
				EnemyBullet_Destroy(i);
				PlayAudio(m_minExSeId, false);
				m_DamageInvincibleTimer = 1.0; // 無敵開始(定数を直書き)
				break; // 1フレーム1回だけダメージ
			}
		}
	}
}

void GameScene::CheckEnemyBulletVsMap()
{
	// エネミー弾とオブジェクトの当たり判定
	for (int j = 0; j < Map_GetObjectsCount(); j++) {
		for (int i = EnemyBullet_GetBulletCount() - 1; i >= 0; i--) {
			Sphere bullet = EnemyBullet_GetSphere(i);
			AABB object = Map_GetObject(j)->Aabb;
			if (Collision_IsOverlapSphereAABB(bullet, object)) {
				BulletHitEffect_Create(EnemyBullet_GetPosition(i));
				EnemyBullet_Destroy(i);
				PlayAudio(m_minExSeId, false);
			}
		}
	}
}
