// ----------------------------------------------------
// プレイヤー [player.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-10-31
// Version: 1.0
// ----------------------------------------------------
#pragma once

#include <DirectXMath.h>
#include "collision.h"

// 前方宣言
struct TerrainMesh;
struct MODEL;

class Player
{
private:
    // シングルトン用のコンストラクタ隠蔽
    Player() = default;
    ~Player() = default;

    // コピー禁止
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

public:
    // シングルトンインスタンスの取得
    static Player& GetInstance()
    {
        static Player instance;
        return instance;
    }

    void Initialize(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& front);
    void Finalize();
    void Update(double elapsed_time);
    void Draw();

    // シャドウマップ描画関数（ライトからの描画）
    void DrawShadow();

    const DirectX::XMFLOAT3& GetPosition() const;
    const DirectX::XMFLOAT3& GetDrawPosition() const; // 描画用の補間された座標取得
    const DirectX::XMFLOAT3& GetFront() const;
    const DirectX::XMFLOAT3& GetVelocity() const; // プレイヤーの速度取得

    AABB GetAABB() const;
    AABB ConvertPositionToAABB(const DirectX::XMVECTOR& position) const;

    // 地形との衝突判定（カプセル判定用）
    void SetTerrainMesh(TerrainMesh* terrain);

    // HP関連の関数
    int GetHP() const;
    int GetMaxHP() const;
    void Damage(int damage);
    bool IsDead() const;

    // 衝突判定Sphere取得
    Sphere GetSphere() const;

    // 左右の操作反転トグル（Ｚキーで切り替え用に呼ぶ）
    void ToggleInvertSteering();

    // 現在の設定状態を取得（デバッグ表示UIで使う）
    bool IsSteeringInverted() const;

private:
    // 状態（メンバ変数）
    DirectX::XMFLOAT3 m_Position{};
    DirectX::XMFLOAT3 m_Front{ 0.0f, 0.0f, 1.0f }; // プレイヤーの前方
    DirectX::XMFLOAT3 m_Velocity{};
    DirectX::XMFLOAT3 m_DrawPosition{};
    DirectX::XMFLOAT3 m_DrawFront{ 0.0f, 0.0f, 1.0f };
    MODEL* m_pModel{ nullptr };

    // ショットSE ID
    int m_ShotSfxId = -1;

    // ショットのクールタイム
    double m_ShotCooldownTimer = 0.0;
    static constexpr double SHOT_COOLDOWN_TIME = 1.5;

    // HP関連
    static constexpr int MAX_HP = 3; // 最大HP(3回で死亡)
    int m_Hp = MAX_HP;

    // 地形衝突判定
    TerrainMesh* m_pTerrainMesh{ nullptr };
    bool m_UseTerrainCollision = false;

    // 定数群
    static constexpr float HALF_WIDTH_X = 0.9f;
    static constexpr float HALF_WIDTH_Z = 1.20f;
    static constexpr float HEIGHT_Y = 1.60f;
    static constexpr float FORWARD_OFFSET = 0.0f;

    static constexpr float COLLISION_RADIUS = 1.0f;
    static constexpr float CAPSULE_HEIGHT = 1.5f;

    static constexpr float WATER_SURFACE_Y = 0.0f;
    static constexpr float ROTATION_SPEED = DirectX::XMConvertToRadians(30.0f);

    // マウスクリックの直前状態（トリガー判定用）
    bool m_PrevMouseLeft = false;
    // Enterキーの直前状態（トリガー判定用）
    bool m_PrevEnterKey = false;

    bool m_InvertSteering = false; // 旋回の左右を反転するフラグ

    // 定数群
    static constexpr float BASE_ACCELERATION = 4000.0f / 50.0f;
    static constexpr float DRAG_COEFFICIENT = 10.0f;
    static constexpr float POSITION_LERP_RATE = 0.3f;
    static constexpr float ROTATION_LERP_RATE = 0.2f;
    static constexpr float MODEL_OFFSET_Y = 0.75f;
    static constexpr float BULLET_SPEED = 10.0f;
    static constexpr float SHOT_TARGET_DISTANCE = 50.0f;

    static constexpr float SPECULAR_EXPONENT = 30.0f;

    // 水面関連
    float m_SurfaceSpeedMultiplier = 1.0f;
    static constexpr float SURFACE_SPEED_MIN = 1.2f;
    static constexpr float SURFACE_SPEED_MAX = 2.0f;
    static constexpr float SURFACE_SPEED_ACCEL_TIME = 2.0f;

    bool m_WasBelowWater = false; // 直前まで水中にいたか
    static constexpr float SURFACE_THRESHOLD = 0.2f;
};
