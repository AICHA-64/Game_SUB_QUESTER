// ----------------------------------------------------
// プレイヤー制御 [player.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#include "player.h"
#include "Audio.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "model.h"
#include "key_logger.h"
#include "light.h"
#include "camera.h"
#include "player_camera.h"
#include "collision.h"
#include "cube.h"
#include "map.h"
#include "bullet.h"
#include "debug_draw.h"
#include "terrain_collision.h"
#include "mouse.h"
#include "gamepad.h"
#include "player_upgrade.h"
#include "surface_transition_effect.h"
#include <cmath>


// ショット音のID

// ショットのクールタイム

// HP関連

// 地形当たり判定用


// 球体/カプセル用パラメータ


// プレイヤーの回転速度
// 小さくすると回転が遅くなる

// マウス左クリックの前フレーム状態（トリガー検出用）
// Enterキーの前フレーム状態（トリガー検出用）


// 水上加速関連


void Player::Initialize(const XMFLOAT3& position, const XMFLOAT3& front)
{
	m_Position = position;
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	XMStoreFloat3(&m_Front, XMVector3Normalize(XMLoadFloat3(&front)));

	m_DrawPosition = position;
	m_DrawFront = m_Front;

	// HPを初期化（強化ボーナス込み）
	m_Hp = MAX_HP + PlayerUpgrade_GetMaxHPBonus();

	m_pModel = ModelLoad("resource/model/player.fbx", 0.005f, false);
	
	m_ShotSfxId = LoadAudio("resource/audio/bullet.wav");
	
	m_pTerrainMesh = nullptr;
	m_UseTerrainCollision = false;
	
	m_PrevMouseLeft = false;
	m_PrevEnterKey = false;
	
	// 水上加速倍率を初期化
	m_SurfaceSpeedMultiplier = 1.0f;
	
	// 水面通過検出の初期化
	m_WasBelowWater = (position.y < WATER_SURFACE_Y - SURFACE_THRESHOLD);
}

void Player::Finalize()
{
	ModelRelease(m_pModel);
	
	if (m_ShotSfxId != -1) {
		UnloadAudio(m_ShotSfxId);
		m_ShotSfxId = -1;
	}
	
	m_pTerrainMesh = nullptr;
}

// 地形メッシュを設定
void Player::SetTerrainMesh(TerrainMesh* terrain)
{
	m_pTerrainMesh = terrain;
	m_UseTerrainCollision = (terrain != nullptr);
}

void Player::Update(double elapsed_time)
{
	// ショットのクールタイム更新
	if (m_ShotCooldownTimer > 0.0) {
		m_ShotCooldownTimer -= elapsed_time;
		if (m_ShotCooldownTimer < 0.0) m_ShotCooldownTimer = 0.0;
	}
	
	XMVECTOR position = XMLoadFloat3(&m_Position);
	XMVECTOR velocity = XMLoadFloat3(&m_Velocity);

	// 水上判定（水面にいるかどうか）
	bool isOnSurface = (m_Position.y >= WATER_SURFACE_Y - 0.1f);

	XMVECTOR direction{};
	
	// カメラの前方向を取得（水平成分のみ）
	XMFLOAT3 cameraFrontF3 = PlayerCamera_GetFront();
	XMVECTOR camera_front = XMLoadFloat3(&cameraFrontF3);
	XMVECTOR camera_front_horizontal = camera_front * XMVECTOR{ 1.0f, 0.0f, 1.0f };
	camera_front_horizontal = XMVector3Normalize(camera_front_horizontal);
	
	// カメラの右方向を計算（前方向 × 上方向 = 右方向）
	XMVECTOR camera_right = XMVector3Cross(camera_front_horizontal, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
	camera_right = XMVector3Normalize(camera_right);

	// Zキーまたはゲームパッドのボタンで操舵モード切替
	if (KeyLogger_IsTrigger(KK_Z) || Gamepad_IsTrigger(GP_BUTTON_Y)) {
		ToggleInvertSteering();
	}

	// 前後移動用のフラグ
	bool movingForward = false;
	bool movingBackward = false;

	// ゲームパッドのスティック入力を取得
	float stickX = Gamepad_GetLeftStickX();
	float stickY = Gamepad_GetLeftStickY();

	// 前後移動 (W/S または 左スティックY軸) - カメラの向きに基づく
	if (KeyLogger_IsPressed(KK_W) || stickY > 0.1f) {
		float intensity = KeyLogger_IsPressed(KK_W) ? 1.0f : stickY;
		direction += camera_front_horizontal * intensity;
		movingForward = true;
	}

	if (KeyLogger_IsPressed(KK_S) || stickY < -0.1f) {
		float intensity = KeyLogger_IsPressed(KK_S) ? 1.0f : -stickY;
		direction -= camera_front_horizontal * intensity;
		movingBackward = true;
	}

	// 左右移動 (A/D または 左スティックX軸) - カメラの向きに基づく
	if (KeyLogger_IsPressed(KK_D) || stickX > 0.1f) {
		float intensity = KeyLogger_IsPressed(KK_D) ? 1.0f : stickX;
		direction -= camera_right * intensity;
	}

	if (KeyLogger_IsPressed(KK_A) || stickX < -0.1f) {
		float intensity = KeyLogger_IsPressed(KK_A) ? 1.0f : -stickX;
		direction += camera_right * intensity;
	}

	// 上下移動 (Q/E または LT/RT トリガー)
	float upDown = 0.0f;
	if (KeyLogger_IsPressed(KK_Q)) {
		upDown += 1.0f;
	}
	if (KeyLogger_IsPressed(KK_E)) {
		upDown -= 1.0f;
	}
	// LT: 浮上 RT: 潜水
	upDown += Gamepad_GetLeftTrigger();
	upDown -= Gamepad_GetRightTrigger();
	upDown = fmaxf(-1.0f, fminf(1.0f, upDown));

	if (upDown != 0.0f) {
		direction += XMVECTOR{ 0.0f, upDown, 0.0f };
	}

	if (XMVectorGetX(XMVector3LengthSq(direction)) > 0.0f) {
		direction = XMVector3Normalize(direction);

		// 水平方向の向き更新（プレイヤーモデルの向き）
		// 移動方向にプレイヤーの向きを徐々に合わせる
		XMVECTOR horizontal_dir = direction * XMVECTOR{ 1.0f, 0.0f, 1.0f };
		if (XMVectorGetX(XMVector3LengthSq(horizontal_dir)) > 0.0f) {
			horizontal_dir = XMVector3Normalize(horizontal_dir);

			// 2つのベクトルがなす角
			float dot = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_Front), horizontal_dir));
			dot = fmaxf(-1.0f, fminf(1.0f, dot));
			float angle = acosf(dot);

			// 回転速度
			const float rot_speed = ROTATION_SPEED * (float)elapsed_time;

			if (angle < rot_speed) {
				XMStoreFloat3(&m_Front, horizontal_dir);
			}
			else {
				XMVECTOR currentFront = XMLoadFloat3(&m_Front);
				XMVECTOR cross = XMVector3Cross(currentFront, horizontal_dir);
				float crossY = XMVectorGetY(cross);

				// 外積の結果（Y成分）に基づいて回転方向を決める
				const float EPSILON = 0.0001f;
				XMMATRIX r = XMMatrixIdentity();

				if (crossY < -EPSILON) {
					r = XMMatrixRotationY(-ROTATION_SPEED);
					XMStoreFloat3(&m_Front, XMVector3TransformNormal(currentFront, r));
				}
				else if (crossY > EPSILON) {
					r = XMMatrixRotationY(ROTATION_SPEED);
					XMStoreFloat3(&m_Front, XMVector3TransformNormal(currentFront, r));
				}
				else {
					XMStoreFloat3(&m_Front, horizontal_dir);
				}
			}
		}

		// 移動速度に強化倍率を適用
		float speedMultiplier = PlayerUpgrade_GetMoveSpeedMultiplier();
		
		// 水上にいる場合は追加の速度倍率を適用（徐々に加速）
		if (isOnSurface) {
			// 水上加速倍率を徐々に上げる
			float accelRate = (SURFACE_SPEED_MAX - SURFACE_SPEED_MIN) / SURFACE_SPEED_ACCEL_TIME;
			m_SurfaceSpeedMultiplier += accelRate * (float)elapsed_time;
			m_SurfaceSpeedMultiplier = fminf(m_SurfaceSpeedMultiplier, SURFACE_SPEED_MAX);
			
			// 初期値より低い場合は初期値に設定
			if (m_SurfaceSpeedMultiplier < SURFACE_SPEED_MIN) {
				m_SurfaceSpeedMultiplier = SURFACE_SPEED_MIN;
			}
			
			speedMultiplier *= m_SurfaceSpeedMultiplier;
		}
		else {
			// 水中にいる場合は加速倍率をリセット
			m_SurfaceSpeedMultiplier = 1.0f;
		}
		
		velocity += direction * (float)(BASE_ACCELERATION * speedMultiplier * (float)elapsed_time);
	}
	else {
		// 移動入力がない場合は水上加速をリセット
		m_SurfaceSpeedMultiplier = 1.0f;
	}

	velocity += -velocity * (float)(DRAG_COEFFICIENT * (float)elapsed_time); // 慣性（水の抵抗）
	position += velocity * (float)elapsed_time;

	// 水面より上には移動不可 潜水艦なので
	if (XMVectorGetY(position) > WATER_SURFACE_Y) {
		position = XMVectorSetY(position, WATER_SURFACE_Y);
		float vy = XMVectorGetY(velocity);
		velocity = XMVectorSetY(velocity, fminf(0.0f, vy)); // 上向き速度をゼロに
	}

	// 地形メッシュとの当たり判定（三角メッシュ）
	if (m_UseTerrainCollision && m_pTerrainMesh)
	{
		XMFLOAT3 pos, vel;
		XMStoreFloat3(&pos, position);
		XMStoreFloat3(&vel, velocity);
		
		// カプセル衝突判定で押し戻し
		TerrainCollision_ResolveCapsuleCollision(
			m_pTerrainMesh,
			pos,
			CAPSULE_HEIGHT,
			COLLISION_RADIUS,
			vel
		);
		
		position = XMLoadFloat3(&pos);
		velocity = XMLoadFloat3(&vel);
	}

	// オブジェクトとの当たり判定
	for (int i = 0; i < Map_GetObjectsCount(); i++) {

		AABB player = ConvertPositionToAABB(position);
		AABB object = Map_GetObject(i)->Aabb;
		Hit hit = Collision_IsHitAABB(object, player);

		// 当たり判定がヒットしたら反発
		if (hit.is_Hit) {
			if (hit.normal.x > 0.0f) {
				position = XMVectorSetX(position, object.max.x + HALF_WIDTH_X);
				velocity *= XMVECTOR{ 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.x < 0.0f) {
				position = XMVectorSetX(position, object.min.x - HALF_WIDTH_X);
				velocity *= XMVECTOR{ 0.0f, 1.0f, 1.0f };
			}
			else if (hit.normal.y > 0.0f) {
				position = XMVectorSetY(position, object.max.y);
				velocity *= XMVECTOR{ 1.0f, 0.0f, 1.0f };
			}
			else if (hit.normal.y < 0.0f) {
				position = XMVectorSetY(position, object.min.y - HEIGHT_Y);
				velocity *= XMVECTOR{ 1.0f, 0.0f, 1.0f };
			}
			else if (hit.normal.z > 0.0f) {
				position = XMVectorSetZ(position, object.max.z + HALF_WIDTH_Z);
				velocity *= XMVECTOR{ 1.0f, 1.0f, 0.0f };
			}
			else if (hit.normal.z < 0.0f) {
				position = XMVectorSetZ(position, object.min.z - HALF_WIDTH_Z);
				velocity *= XMVECTOR{ 1.0f, 1.0f, 0.0f };
			}
		}
	}

	XMStoreFloat3(&m_Position, position);
	XMStoreFloat3(&m_Velocity, velocity);

	// 水面通過検出とエフェクトトリガー
	bool isNowBelowWater = (m_Position.y < WATER_SURFACE_Y - SURFACE_THRESHOLD);
	
	if (m_WasBelowWater && !isNowBelowWater) {
		// 水中→水上（浮上）
		SurfaceTransitionEffect_TriggerSurface(m_Position);
	}
	else if (!m_WasBelowWater && isNowBelowWater) {
		// 水上→水中（潜航）
		XMFLOAT3 cameraFront = PlayerCamera_GetFront();
		SurfaceTransitionEffect_TriggerDive(m_Position, cameraFront);
	}
	
	m_WasBelowWater = isNowBelowWater;

	// 描画用の位置を補間して滑らかにする
	XMVECTOR drawPos = XMLoadFloat3(&m_DrawPosition);
	XMVECTOR targetPos = XMLoadFloat3(&m_Position);
	drawPos = XMVectorLerp(drawPos, targetPos, POSITION_LERP_RATE);
	XMStoreFloat3(&m_DrawPosition, drawPos);

	// 描画用の向きを補間して滑らかにする
	XMVECTOR drawFront = XMLoadFloat3(&m_DrawFront);
	XMVECTOR targetFront = XMLoadFloat3(&m_Front);
	drawFront = XMVectorLerp(drawFront, targetFront, ROTATION_LERP_RATE);
	drawFront = XMVector3Normalize(drawFront);
	XMStoreFloat3(&m_DrawFront, drawFront);

	// 射撃入力（マウス左クリック / Enter / ゲームパッドA or RB）
	Mouse_State ms{};
	Mouse_GetState(&ms);
	
	bool enterKeyPressed = KeyLogger_IsPressed(KK_ENTER);
	bool gamepadShoot = Gamepad_IsTrigger(GP_BUTTON_A) || Gamepad_IsTrigger(GP_BUTTON_RB);
	bool shootTrigger = false;
	
	// マウス左クリックのトリガー検出
	if (ms.leftButton && !m_PrevMouseLeft) {
		shootTrigger = true;
	}
	// Enterキーのトリガー検出
	if (enterKeyPressed && !m_PrevEnterKey) {
		shootTrigger = true;
	}
	// ゲームパッドのトリガー検出
	if (gamepadShoot) {
		shootTrigger = true;
	}

	// クールタイムが切れている
	if (shootTrigger && m_ShotCooldownTimer <= 0.0) {
		XMFLOAT3 bullet_velocity;
		// カメラ位置を取得
		XMFLOAT3 camera_position = PlayerCamera_GetPosition();

		// 発射位置を下にオフセット（視界を遮らないように）
		XMFLOAT3 shot_position = camera_position;
		shot_position.y -= 0.5f;

		// 画面中央（レティクル位置）への方向ベクトルを計算
		// カメラ位置から前方に延長した点を目標とする
		XMFLOAT3 cameraFront = PlayerCamera_GetFront();
		XMVECTOR target_point = XMLoadFloat3(&camera_position) + XMLoadFloat3(&cameraFront) * SHOT_TARGET_DISTANCE;

		// 発射位置から目標点への方向を計算
		XMVECTOR shot_pos_vec = XMLoadFloat3(&shot_position);
		XMVECTOR shotDirection = XMVector3Normalize(target_point - shot_pos_vec);

		// 弾速に強化倍率を適用
		float bulletSpeedMultiplier = PlayerUpgrade_GetBulletSpeedMultiplier();
		XMStoreFloat3(&bullet_velocity, shotDirection * BULLET_SPEED * bulletSpeedMultiplier);

		Bullet_Create(shot_position, bullet_velocity);

		// ショット音を再生
		if (m_ShotSfxId != -1) PlayAudio(m_ShotSfxId, false);
		
		// クールタイム開始（強化倍率適用）
		float intervalMultiplier = PlayerUpgrade_GetShotIntervalMultiplier();
		m_ShotCooldownTimer = SHOT_COOLDOWN_TIME * intervalMultiplier;
		
		// 射撃時に短い振動フィードバック（カメラシェイクの振動とは別に一瞬だけ）
		Gamepad_SetVibration(0.5f, 0.5f);
	}
	// 振動の停止はplayer_camera.cppのカメラシェイク処理で管理

	m_PrevMouseLeft = ms.leftButton;
	m_PrevEnterKey = enterKeyPressed;
}

void Player::Draw()
{
	Light_SetSpecularWorld(PlayerCamera_GetPosition(), SPECULAR_EXPONENT, { 1.0f, 1.0f, 1.0f, 1.0f });

	float angle = -atan2f(m_DrawFront.z, m_DrawFront.x) + XMConvertToRadians(270);

	XMMATRIX r = XMMatrixRotationY(angle);

	XMMATRIX translation = XMMatrixTranslation(m_DrawPosition.x, m_DrawPosition.y + MODEL_OFFSET_Y, m_DrawPosition.z);
	XMMATRIX world = r * translation;
	// ModelDraw(m_pModel, world);

#ifdef _DEBUG
	DebugDraw_AABB(GetAABB(), { 1.0f, 1.0f, 0.0f, 1.0f });
#endif // _DEBUG
}

// シャドウマップ専用描画関数（深度のみ）
void Player::DrawShadow()
{
	if (!m_pModel) return;
	
	float angle = -atan2f(m_DrawFront.z, m_DrawFront.x) + XMConvertToRadians(270);
	XMMATRIX r = XMMatrixRotationY(angle);
	XMMATRIX translation = XMMatrixTranslation(m_DrawPosition.x, m_DrawPosition.y + MODEL_OFFSET_Y, m_DrawPosition.z);
	XMMATRIX world = r * translation;
	
	// ModelDrawShadow(m_pModel, world);
}

const DirectX::XMFLOAT3& Player::GetPosition() const
{
	return m_Position;
}

const DirectX::XMFLOAT3& Player::GetDrawPosition() const
{
	return m_DrawPosition;
}

const DirectX::XMFLOAT3& Player::GetFront() const
{
	return m_Front;
}

const DirectX::XMFLOAT3& Player::GetVelocity() const
{
	return m_Velocity;
}

AABB Player::GetAABB() const
{
	XMVECTOR pos = XMLoadFloat3(&m_Position);
	XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&m_Front));
	pos += front * FORWARD_OFFSET;

	XMFLOAT3 c; XMStoreFloat3(&c, pos);
	return {
		{ c.x - HALF_WIDTH_X, c.y,                   c.z - HALF_WIDTH_Z },
		{ c.x + HALF_WIDTH_X, c.y + HEIGHT_Y, c.z + HALF_WIDTH_Z }
	};
}

AABB Player::ConvertPositionToAABB(const DirectX::XMVECTOR& position) const
{
	XMVECTOR front = XMVector3Normalize(XMLoadFloat3(&m_Front));
	XMVECTOR center = position + front * FORWARD_OFFSET;

	AABB aabb;
	XMStoreFloat3(&aabb.min, center - XMVECTOR{ HALF_WIDTH_X, 0.0f,            HALF_WIDTH_Z });
	XMStoreFloat3(&aabb.max, center + XMVECTOR{ HALF_WIDTH_X, HEIGHT_Y, HALF_WIDTH_Z });
	return aabb;
}

// HP関連の関数実装
int Player::GetHP() const
{
	return m_Hp;
}

int Player::GetMaxHP() const
{
	// 強化ボーナス込みの最大HP
	return MAX_HP + PlayerUpgrade_GetMaxHPBonus();
}

void Player::Damage(int damage)
{
	m_Hp -= damage;
	if (m_Hp < 0) {
		m_Hp = 0;
	}
	PlayerCamera_Shake(0.5f); // ダメージ時にカメラを揺らす;
}

bool Player::IsDead() const
{
	return m_Hp <= 0;
}

Sphere Player::GetSphere() const
{
	return { m_Position, COLLISION_RADIUS };
}

void Player::ToggleInvertSteering()
{
	m_InvertSteering = !m_InvertSteering;
}

bool Player::IsSteeringInverted() const
{
	return m_InvertSteering;
}

