// ----------------------------------------------------
// タイトル制御 [title.cpp]
// ====================================================

#include "title.h"
#include "sprite.h"
#include "texture.h"
#include "key_logger.h"
#include "gamepad.h"
#include "fade.h"
#include "scene.h"
#include "direct3d.h"
#include "Audio.h"
#include "game_window.h"
#include "player_upgrade.h"
#include "mouse.h"

void TitleScene::Initialize()
{
	m_TitleTexId = Texture_Load(L"resource/texture/Title.png");
	
	m_AccumulatedTime = 0.0;

	// タイトル画面に戻ったら全ての強化とレベルをリセット
	PlayerUpgrade_Reset();

	// BGM読み込み・再生
	m_BgmId = LoadAudio("resource/audio/sea.wav");
	if (m_BgmId != -1) {
		PlayAudio(m_BgmId, true);
	}

	Fade_Start(3.0, false);
}

void TitleScene::Finalize()
{
	// BGM停止・解放
	if (m_BgmId != -1) {
		StopAudio(m_BgmId);
		UnloadAudio(m_BgmId);
		m_BgmId = -1;
	}
}

void TitleScene::Update(double elapsed_time)
{
	// ゲームパッド更新
	Gamepad_Update();
	
	m_AccumulatedTime += elapsed_time;
	
	Mouse_State ms;
	Mouse_GetState(&ms);
	bool mouseLeftTrigger = (ms.leftButton && !m_PrevMouseLeft);
	m_PrevMouseLeft = ms.leftButton;
	
	
	// フルスクリーン切り替え（Pキーまたはスタートボタン）
	if (KeyLogger_IsTrigger(KK_P) || Gamepad_IsTrigger(GP_BUTTON_START)) {
		GameWindow_ToggleFullScreen();
	}
	
	// ゲーム終了（Escキーまたはバックボタン）
	if (KeyLogger_IsTrigger(KK_ESCAPE) || Gamepad_IsTrigger(GP_BUTTON_BACK)) {
		PostQuitMessage(0);
	}
	
	// Enterキー、コントローラーAボタン、または左クリックで開始
	if (KeyLogger_IsTrigger(KK_ENTER) || Gamepad_IsTrigger(GP_BUTTON_A) || mouseLeftTrigger)
	{
		Fade_Start(1.0, true);
	}

	if (Fade_GetState() == FADE_STATE_FINISHED_OUT)
	{
		Scene_Change(SCENE_GAME);
	}
}

void TitleScene::Draw()
{
	Direct3D_SetDepthEnable(false);
	Sprite_Begin();
	
	float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
	float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
	
	// タイトル画像を全画面に表示
	if (m_TitleTexId >= 0) {
		Sprite_Draw(m_TitleTexId, 0.0f, 0.0f, screenWidth, screenHeight);
	}
}
