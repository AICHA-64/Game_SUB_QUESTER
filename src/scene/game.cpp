// ----------------------------------------------------
// 
// ゲーム本体 [game.cpp]
// 
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-06-27
// ----------------------------------------------------
#include "game.h"
#include "cube.h"
#include "light.h"
#include <DirectXMath.h>
using namespace DirectX;
#include "key_logger.h"
#include "sampler.h"
#include "meshfield.h"
#include "model.h"
#include "player.h"
#include "bullet.h"
#include "player_camera.h"
#include "map.h"
#include "billboard.h"
#include "texture.h"
#include "sprite_anim.h"
#include "camera.h"
#include "bullet_hit_effect.h"
#include "direct3d.h"
#include "trajectory3d.h"
#include "sky.h"
#include "Enemy.h"
#include "enemy_normal.h"
#include "enemy_final_boss.h"
#include "enemy_bullet.h"
#include "mouse.h"
#include "sprite.h"
#include "map_camera.h"
#include "shader_shadow.h"
#include "light_camera.h"
#include "shader3d.h"
#include "shader_field.h"
#include "route_search.h"
#include "water.h"
#include "water_reflect.h"
#include "debug_draw.h"
#include "underwater_effect.h"
#include "fade.h"
#include "Audio.h"
#include "scene.h"
#include "game_window.h"
#include "shot_counter.h"
#include "timer.h"
#include "boss_counter.h"
#include "particle_test.h"
#include "bloom.h"
#include "gamepad.h"
#include "player_upgrade.h"
#include "surface_transition_effect.h"
#include "game_data.h"
#include <cmath>  // atan2f, cosf, sinf, sqrtf用
#include <cfloat> // FLT_MAX用


// タイム表示用
static constexpr unsigned int TIME_CHIP_WIDTH = 32;
static constexpr unsigned int TIME_CHIP_HEIGHT = 32;
static constexpr int TIME_FONT_WIDTH = 64;
static constexpr int TIME_FONT_HEIGHT = 64;

// 強化状態表示用
static constexpr float STATUS_ICON_HEIGHT = 40.0f;
static constexpr float STATUS_ICON_WIDTH = 100.0f;
static constexpr float STATUS_PADDING = 10.0f;
static constexpr float STATUS_LINE_HEIGHT = 50.0f;

void StopAudio(int Index);

WaterRT g_WaterRT;

// 水面反射用のビュー・プロジェクション行列
DirectX::XMFLOAT4X4 BuildReflectionViewProj(const DirectX::XMFLOAT3& cameraPos,
const DirectX::XMFLOAT3& cameraFront,
float waterHeight,
float aspect, float fov, float nearZ, float farZ);

// BGM切り替えの高さ閾値
static constexpr float BGM_THRESHOLD = -1.0f;

// ダメージ後の無敵時間
static constexpr double DAMAGE_INVINCIBLE_TIME = 1.0; // 1秒間無敵

#ifdef _DEBUG
// デバッグ用:0キー連打でボス全滅
static constexpr int DEBUG_BOSS_KILL_KEY_COUNT = 3; // 3回押しでボス全滅
#endif


void GameScene::Initialize()
{
	Gamepad_Initialize(); // ゲームパッド初期化

	// プレイヤーのスポーン位置と向きを決定
	DirectX::XMFLOAT3 playerSpawnPos;
	DirectX::XMFLOAT3 playerSpawnFront;
	
	// ラウンド数を取得(1回目は0、2回目以降は1以上)
	int currentRound = PlayerUpgrade_GetRound();
	
	// ラスボス戦で死んでリトライする場合は、通常戦に戻る
	bool isFinalBossBattle = false;
	if (GameData_GetDiedInFinalBoss()) {
		// ラスボス戦で死亡 -> 通常戦に戻る（強化は保持）
		isFinalBossBattle = false;
		// フラグクリア
		GameData_SetDiedInFinalBoss(false);
	} else if (currentRound > 0) {
		// 2回目以降はラスボス戦の判定
		isFinalBossBattle = Enemy_RollFinalBossBattle();
		if (isFinalBossBattle) {
			// ラスボス戦確定
			Enemy_ResetFinalBossChance();
		}
	}
	Enemy_SetFinalBossMode(isFinalBossBattle);
	
#if defined(_DEBUG)
	// デバッグ出力
	{
		char buf[128];
		sprintf_s(buf, "Round: %d, FinalBoss Chance: %d%%, FinalBoss Battle: %s\n", 
			currentRound, Enemy_GetFinalBossChance(), isFinalBossBattle ? "YES" : "NO");
		OutputDebugStringA(buf);
	}
#endif
	
	if (currentRound == 0 || isFinalBossBattle) {
		// 初回またはラスボス戦は固定
		playerSpawnPos = { 0.0f, 0.0f, -5.0f };
		playerSpawnFront = { 0.0f, 0.0f, 1.0f };
	} else {
		// 2回目以降向きのみランダム
		playerSpawnPos = { 0.0f, 0.0f, -5.0f }; 
		
		// ランダムな向きを生成(0～360度)
		float randomAngle = (float)rand() / (float)RAND_MAX * 2.0f * 3.14159265f;
		
		playerSpawnFront.x = cosf(randomAngle);
		playerSpawnFront.y = 0.0f;
		playerSpawnFront.z = sinf(randomAngle);
	}

	Player::GetInstance().Initialize(playerSpawnPos, playerSpawnFront);

	Enemy_Initialize();
	EnemyBullet_Initialize();
	
	Camera_Initialize(
		{ 7.0f, 10.0f, -9.0f },
		{-0.63f, -0.45f, 0.63f},
		{0.7f, 0.0f, 0.7f});
		
	PlayerCamera_Initialize();
	Map_Initialize();
	RouteSearch_Initialize();
	Bullet_Initialize();
	Sky_Initialize();
	Water_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	Billboard_Initialize();

	int w = (int)Direct3D_GetBackBufferWidth();
	int h = (int)Direct3D_GetBackBufferHeight();
	WaterRT_Create(Direct3D_GetDevice(), w, h, g_WaterRT);

	BulletHitEffect_Initialize();
	Trajectory3D_Initialize();
	
	// 水面通過エフェクトの初期化
	SurfaceTransitionEffect_Initialize();

	m_Emitter = new SplashEmitter(5000, { 0.0f, 0.0f, 0.0f }, 3000.0, true);

	// モードに応じて敵を配置
	if (isFinalBossBattle) {
		// ラスボス戦: ラスボスを1体配置(中央に)
		Enemy_CreateFinalBoss({ 0.0f, -10.0f, 90.0f });
#if defined(_DEBUG)
		OutputDebugStringA("=== FINAL BOSS BATTLE ===\n");
#endif
	} else {
		// 通常戦: 通常ボスを4体配置
		Enemy_CreateBoss({ 40.0f, -10.0f, -85.0f });
		Enemy_CreateBoss({ -60.0f, -5.0f, 90.0f });
		Enemy_CreateBoss({ 100.0f, -5.0f, 20.0f });
		Enemy_CreateBoss({ -120.0f, -10.0f, 30.0f });
	}

	// テクスチャ読み込み
	m_UITexId = Texture_Load(L"resource/texture/UI.png");
	m_UITex2Id = Texture_Load(L"resource/texture/UI_text.png");
	m_MapTexId = Texture_Load(L"resource/texture/map.png");
	m_reticleTexId = Texture_Load(L"resource/texture/reticle.png");

	m_TimeTexId = Texture_Load(L"resource/texture/TIME.png");
	m_ScoreTexId = Texture_Load(L"resource/texture/SCORE_UI.png");
	m_WasdUITexId = Texture_Load(L"resource/texture/WASD.png");
	m_TargetTexId = Texture_Load(L"resource/texture/TARGET.png");
	m_TimeChipTexId = Texture_Load(L"resource/texture/chip1.png");
	m_WhiteTexId = Texture_Load(L"resource/texture/white.png");

	m_RadarLineTexId = Texture_Load(L"resource/texture/white.png");

	m_UpgradeTexIds[UPGRADE_MOVE_SPEED] = Texture_Load(L"resource/texture/Speed.png");
	m_UpgradeTexIds[UPGRADE_SHOT_INTERVAL] = Texture_Load(L"resource/texture/Bullet.png");
	m_UpgradeTexIds[UPGRADE_MAX_HP] = Texture_Load(L"resource/texture/HP.png");
	m_UpgradeTexIds[UPGRADE_BULLET_SPEED] = Texture_Load(L"resource/texture/Shot.png");
	m_UpgradeTexIds[UPGRADE_DAMAGE] = Texture_Load(L"resource/texture/Power.png");

	m_IsDebug = false;
	m_DamageInvincibleTimer = 0.0;

	ShaderShadow_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	LightCamera_Initialize();

	DebugDraw_Initialize(Direct3D_GetDevice(), Direct3D_GetContext());
	
	// 水中エフェクトの初期化
	UnderwaterEffect_Initialize();

	ShotCounter_Initialize(1700.0f, 110.0f, 3); // 射撃カウンターの初期化
	
	BossCounter_Initialize(1500.0f, 110.0f, 2); // ボスカウンターの初期化

	Timer_Initialize(10.0f, 80.0f); // タイマーを左上に初期化

	// Bloom初期化
	Direct3D_CreateMainSceneRT(w, h);
	Bloom_Initialize(Direct3D_GetDevice(), Direct3D_GetContext(), w, h);
	
	// 0.広範囲
	Bloom_SetLayerParameters(0, 0.1f, 0.6f);
	
	// 1.ハイライト層（強い光源のみ抽出）
	Bloom_SetLayerParameters(1, 0.8f, 2.0f);
	
	//2.中間層
	Bloom_SetLayerParameters(2, 0.9f, 5.1f);

	// BGM読み込み（地上/水中）
	m_BgmId = LoadAudio("resource/audio/sea.wav");
	m_UnderseaBgmId = LoadAudio("resource/audio/undersea.wav");

	// SEの読み込み
	m_minExSeId = LoadAudio("resource/audio/miniex.wav");

	// プレイヤーの高さに応じて再生するBGMを切り替え
	{
		auto pos = Player::GetInstance().GetPosition();
		if (pos.y <= BGM_THRESHOLD) {
			PlayAudio(m_UnderseaBgmId, true);
			m_CurrentBgmPlaying = m_UnderseaBgmId;
		} else {
			PlayAudio(m_BgmId, true);
			m_CurrentBgmPlaying = m_BgmId;
		}
	}

	// 反射パス用のラスタライザステート
	{
		ID3D11Device* dev = Direct3D_GetDevice();
		if (dev) {
			D3D11_RASTERIZER_DESC rd = {};
			rd.FillMode = D3D11_FILL_SOLID;
			rd.CullMode = D3D11_CULL_NONE; // カリング無効
			rd.DepthClipEnable = TRUE;
			rd.MultisampleEnable = FALSE;
			dev->CreateRasterizerState(&rd, &m_pReflectionNoCullRS);
		}
	}

	Fade_Start(2.0, false);
	m_GameStart = false;
	m_IsGameFinished = false;
	GameData_SetDiedInFinalBoss(false); // フラグを初期化
}

void GameScene::Finalize()
{
	Gamepad_Finalize(); // ゲームパッド終了

	delete m_Emitter;
	m_Emitter = nullptr;

	Timer_Finalize();
	ShotCounter_Finalize();
	BossCounter_Finalize();
	UnderwaterEffect_Finalize();
	
	SurfaceTransitionEffect_Finalize();

	Bloom_Finalize();

	DebugDraw_Finalize();

	Trajectory3D_Finalize();
	BulletHitEffect_Finalize();
	Billboard_Finalize();
	WaterRT_Release(g_WaterRT);
	Water_Finalize();
	Sky_Finalize();
	Bullet_Finalize();
	RouteSearch_Finalize();
	Map_Finalize();

	PlayerCamera_Finalize();

	Camera_Finalize();
	
	EnemyBullet_Finalize();
	Enemy_Finalize();

	Player::GetInstance().Finalize();

	ShaderShadow_Finalize();

	// 反射パス用ラスタライザステートの解放
	SAFE_RELEASE(m_pReflectionNoCullRS);

	// BGMアンロード
	if (m_BgmId != -1) {
		UnloadAudio(m_BgmId);
		m_BgmId = -1;
	}
	if (m_UnderseaBgmId != -1) {
		UnloadAudio(m_UnderseaBgmId);
		m_UnderseaBgmId = -1;
	}
}


void GameScene::Update(double elapsed_time)
{
	Gamepad_Update(); // ゲームパッド更新(毎フレーム)

	// Backキーまたはゲームパッドのバックボタンでタイトルに戻る
	if (KeyLogger_IsTrigger(KK_BACK) || Gamepad_IsTrigger(GP_BUTTON_BACK)) {
		// BGMを停止
		if (m_CurrentBgmPlaying != -1) {
			StopAudio(m_CurrentBgmPlaying);
		}
		Scene_Change(SCENE_TITLE);
		return;
	}

	// フェードインが完了したらBGMを再生
	if (m_GameStart && Fade_GetState() == FADE_STATE_FINISHED_IN) {
		PlayAudio(m_BgmId, true);
		m_GameStart = true;
	}

	// プレイヤーが死亡したらフェードアウト開始
	if (!m_IsGameFinished && Player::GetInstance().IsDead()) {
		m_IsGameFinished = true;
		GameData_SetFinalTime(Timer_GetTime()); // 最終タイムを記録
		
		// ラスボス戦モードで死亡した場合はフラグ
		if (Enemy_IsFinalBossMode()) {
			GameData_SetDiedInFinalBoss(true);
		}
		
		Fade_Start(1.0, true); // フェードアウト
	}

	// ラスボス戦モードかどうか
	if (Enemy_IsFinalBossMode()) {
		// ラスボス戦: 倒したら通常戦に戻る（強化は保持）
		if (!m_IsGameFinished && Enemy_IsFinalBossDefeated()) {
			m_IsGameFinished = true;
			GameData_SetFinalTime(Timer_GetTime());
			Fade_Start(1.0, true);
			
			// ラスボス撃破時にレベル上げ
			PlayerUpgrade_IncrementFinalBossLevel();
			
#if defined(_DEBUG)
			char buf[128];
			sprintf_s(buf, "=== FINAL BOSS DEFEATED! Level %d -> %d ===\n", 
				PlayerUpgrade_GetFinalBossLevel() - 1, PlayerUpgrade_GetFinalBossLevel());
			OutputDebugStringA(buf);
#endif
		}
	} else {
		// 通常戦: ボスを1体でも撃破したらドックへ
		if (!m_IsGameFinished && Enemy_IsAnyBossDefeated()) {
			m_IsGameFinished = true;
			GameData_SetFinalTime(Timer_GetTime());
			Fade_Start(1.0, true);
		}
	}

	// フェードアウトが完了したらシーン遷移
	if (m_IsGameFinished && Fade_GetState() == FADE_STATE_FINISHED_OUT) {
		if (Player::GetInstance().IsDead()) {
			// HPが0ならリザルトへ
			Scene_Change(SCENE_RESULT);
		} else if (Enemy_IsFinalBossMode()) {
			// ラスボス撃破なら通常戦に戻る（ドック経由でラウンド増加）
			Scene_Change(SCENE_DOCK);
		} else {
			// 通常ボス撃破ならドック(強化選択)へ
			Scene_Change(SCENE_DOCK);
		}
		return;
	}

	// フルスクリーン切り替え(Pキーまたはスタートボタン)
	if (KeyLogger_IsTrigger(KK_P) || Gamepad_IsTrigger(GP_BUTTON_START)) {
		GameWindow_ToggleFullScreen();
	}
	
	// 無敵時間の更新
	if (m_DamageInvincibleTimer > 0.0) {
		m_DamageInvincibleTimer -= elapsed_time;
	}

	if(m_IsDebug) {
		Camera_Update(elapsed_time);
		// デバッグカメラ時はスカイドームのY成分を固定して上下移動を防ぐ
		XMFLOAT3 camPos = Camera_GetPosition();
		XMFLOAT3 playerPos = Player::GetInstance().GetPosition(); // 地面基準の高さを利用
		camPos.y = playerPos.y;
		Sky_SetPosition(camPos);
	}
	else
	{
		PlayerCamera_Update(elapsed_time);
		Player::GetInstance().Update(elapsed_time);
		Enemy_Update(elapsed_time);
		EnemyBullet_Update(elapsed_time);
		Sky_SetPosition(Player::GetInstance().GetPosition());
	}
	
	Bullet_Update(elapsed_time);

	// 水中エフェクトの更新(プレイヤーのY座標を渡す)
	XMFLOAT3 playerPos = Player::GetInstance().GetPosition();
	UnderwaterEffect_Update(playerPos.y);

	// BGMの切り替え: プレイヤーのyが閾値以下なら水中BGM、以上なら地上BGM
	if (m_BgmId != -1 && m_UnderseaBgmId != -1) {
		if (playerPos.y <= BGM_THRESHOLD) {
			// 水中に切り替え
			if (m_CurrentBgmPlaying != m_UnderseaBgmId) {
				// 現在再生中のBGMを停止してから水中BGMを再生
				if (m_CurrentBgmPlaying != -1) {
					StopAudio(m_CurrentBgmPlaying);
				}
				PlayAudio(m_UnderseaBgmId, true);
				m_CurrentBgmPlaying = m_UnderseaBgmId;
			}
		} else {
			// 地上に切り替え
			if (m_CurrentBgmPlaying != m_BgmId) {
				if (m_CurrentBgmPlaying != -1) {
					StopAudio(m_CurrentBgmPlaying);
				}
				PlayAudio(m_BgmId, true);
				m_CurrentBgmPlaying = m_BgmId;
			}
		}
	}

	CheckBulletMapCollision();
	CheckBulletEnemyCollision();
	CheckFinalBossIntercept();
	CheckBulletVsEnemyBullet();
	CheckEnemyBulletVsPlayer();
	CheckEnemyBulletVsMap();

	Water_Update(elapsed_time);

	SpriteAnim_Update(elapsed_time);

	BulletHitEffect_Update();

	Trajectory3D_Update(elapsed_time);

	// レーダー波の角度更新（1秒で1周）
	m_RadarAngle += (float)elapsed_time * 2.0f * DirectX::XM_PI / 2.0f;
	if (m_RadarAngle > DirectX::XM_PI * 2.0f) {
		m_RadarAngle -= DirectX::XM_PI * 2.0f;
	}
	
	// 水面通過エフェクトの更新
	SurfaceTransitionEffect_Update(elapsed_time);
	
	// 水中での定期的な泡生成
	constexpr float WATER_SURFACE_THRESHOLD = 10.0f;
	bool isUnderwater = (playerPos.y < WATER_SURFACE_THRESHOLD);
	XMFLOAT3 cameraFront = PlayerCamera_GetFront();
	XMFLOAT3 playerVelocity = Player::GetInstance().GetVelocity();
	SurfaceTransitionEffect_UpdateUnderwaterBubbles(elapsed_time, isUnderwater, playerPos, cameraFront, playerVelocity);

	Timer_Update(elapsed_time);

	// ボスカウンターの更新
	BossCounter_SetCount(Enemy_GetAliveBossCount());

#ifdef _DEBUG
	if(KeyLogger_IsTrigger(KK_L)) {
		m_IsDebug = !m_IsDebug;
	}

	// デバッグ用:0キーを3回押したらボス全滅
	if (KeyLogger_IsTrigger(KK_D0)) {
		m_DebugKey0PressCount++;
		char buf[64];
		sprintf_s(buf, "Debug: 0 key pressed %d/%d\n", m_DebugKey0PressCount, DEBUG_BOSS_KILL_KEY_COUNT);
		OutputDebugStringA(buf);
		
		if (m_DebugKey0PressCount >= DEBUG_BOSS_KILL_KEY_COUNT) {
			Enemy_KillAllBosses();
			m_DebugKey0PressCount = 0;
			OutputDebugStringA("Debug: All bosses killed!\n");
		}
	}

	if (KeyLogger_IsTrigger(KK_R)) {
		m_ReflectionDebugMode = (m_ReflectionDebugMode +1) %7;
		char buf[64];
		sprintf_s(buf, "Reflection debug mode: %d\n", m_ReflectionDebugMode);
		OutputDebugStringA(buf);
	}

	// Bloomのテスト
	if (KeyLogger_IsTrigger(KK_D1)) {
		m_BloomThreshold -= 0.1f;
		if (m_BloomThreshold < 0.0f) m_BloomThreshold = 0.0f;
		Bloom_SetLayerParameters(0, m_BloomThreshold, 0.6f);
	}
	if (KeyLogger_IsTrigger(KK_D2)) {
		m_BloomThreshold += 0.1f;
		if (m_BloomThreshold > 2.0f) m_BloomThreshold = 2.0f;
		Bloom_SetLayerParameters(0, m_BloomThreshold, 0.6f);
	}
	if (KeyLogger_IsTrigger(KK_D3)) {
		m_BloomIntensity -= 0.2f;
		if (m_BloomIntensity < 0.0f) m_BloomIntensity = 0.0f;
		Bloom_SetLayerParameters(1, 0.8f, m_BloomIntensity);
	}
	if (KeyLogger_IsTrigger(KK_D4)) {
		m_BloomIntensity += 0.2f;
		if (m_BloomIntensity > 5.0f) m_BloomIntensity = 5.0f;
		Bloom_SetLayerParameters(1, 0.8f, m_BloomIntensity);
	}

	if (KeyLogger_IsTrigger(KK_D5)) {
		m_TerrainShadowIntensity -= 0.1f;
		if (m_TerrainShadowIntensity < 0.0f) m_TerrainShadowIntensity = 0.0f;
	}
	if (KeyLogger_IsTrigger(KK_D6)) {
		m_TerrainShadowIntensity += 0.1f;
		if (m_TerrainShadowIntensity > 1.0f) m_TerrainShadowIntensity = 1.0f;
	}
#endif

	m_Emitter->Update(elapsed_time);
}




