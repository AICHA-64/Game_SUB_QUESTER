// ----------------------------------------------------
// ゲーム本体 [game.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-06-27
// Version: 1.0
// ----------------------------------------------------
#ifndef GAME_H
#define GAME_H

#include "scene.h"
#include "player_upgrade.h"
#include <DirectXMath.h>

struct MODEL;
class NormalEmitter;
class SplashEmitter;
struct ID3D11RasterizerState;

class GameScene : public IScene {
private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_angle = 0.0f;
    float m_scale = 1.0f;
    DirectX::XMFLOAT3 m_lightDirection{ -1.0f, -1.0f, -1.0f };

    MODEL* m_pModelTest = nullptr;
    MODEL* m_pModelTest1 = nullptr;
    MODEL* m_pModelTest2 = nullptr;
    MODEL* m_pModelTest3 = nullptr;

    int m_TestTexId = -1;
    int m_UITexId = -1;
    int m_UITex2Id = -1;
    int m_MapTexId = -1;
    int m_reticleTexId = -1;
    int m_MinimapMaskTexId = -1;
    int m_RadarLineTexId = -1;
    float m_RadarAngle = 0.0f;
    int m_TimeTexId = -1;
    int m_ScoreTexId = -1;
    int m_WasdUITexId = -1;
    int m_TargetTexId = -1;
    int m_TimeChipTexId = -1;
    int m_WhiteTexId = -1;
    int m_UpgradeTexIds[UPGRADE_TYPE_COUNT] = { -1, -1, -1, -1, -1 };

    bool m_IsDebug = false;
    int m_BgmId = -1;
    int m_UnderseaBgmId = -1;
    int m_minExSeId = -1;
    bool m_GameStart = false;
    bool m_IsGameFinished = false;
    
    int m_CurrentBgmPlaying = -1;
    
    double m_DamageInvincibleTimer = 0.0;
    
    NormalEmitter* m_NormalEmitter{};
    SplashEmitter* m_Emitter{};
    
    int m_ReflectionDebugMode = 0;
    ID3D11RasterizerState* m_pReflectionNoCullRS = nullptr;
    
    float m_BloomThreshold = 0.1f;
    float m_BloomIntensity = 0.6f;
    float m_TerrainShadowIntensity = 1.0f;
    
    int m_DebugKey0PressCount = 0;

    void CheckBulletMapCollision();
    void CheckBulletEnemyCollision();
    void CheckFinalBossIntercept();
    void CheckBulletVsEnemyBullet();
    void CheckEnemyBulletVsPlayer();
    void CheckEnemyBulletVsMap();
    void RenderReflectionPass(bool useDebugCamera);
    int GetUpgradeTextureId(UpgradeType type);
    void DrawNumber(float x, float y, int number);
    void DrawUpgradeStatus(float x, float y);
    void mapRendering();
    void shadowMapRendering();
    void DrawUI();

public:
    void Initialize() override;
    void Finalize() override;
    void Update(double elapsed_time) override;
    void Draw() override;
};

#endif // GAME_H
