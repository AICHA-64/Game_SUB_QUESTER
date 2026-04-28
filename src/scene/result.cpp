// ----------------------------------------------------
// リザルト制御 [result.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-08-31
// Version: 1.0
// ----------------------------------------------------

#include "result.h"
#include "sprite.h"
#include "texture.h"
#include "key_logger.h"
#include "gamepad.h"
#include "fade.h"
#include "scene.h"
#include "score.h"
#include "direct3d.h"
#include "game.h"
#include "timer.h"
#include "shot_counter.h"
#include "player_upgrade.h"
#include "Audio.h"
#include "game_window.h"
#include "game_data.h"
#include <float.h>
#include <DirectXMath.h>
#include "mouse.h"

static constexpr unsigned int SCORE_Chip_WIDTH = 32;
static constexpr unsigned int SCORE_Chip_HEIGHT = 32;
static constexpr int SCORE_FONT_WIDTH = 64;
static constexpr int SCORE_FONT_HEIGHT = 64;
static constexpr int SCORE_FONT_WIDTH_SHORT = 32;

// ボタン画像の実際のサイズに合わせて調整
static constexpr float BUTTON_SRC_WIDTH = 500.0f;
static constexpr float BUTTON_SRC_HEIGHT = 150.0f;
// 画面に描画する際のボタンサイズ
static constexpr float BUTTON_DRAW_WIDTH = 500.0f;
static constexpr float BUTTON_DRAW_HEIGHT = 150.0f;



void ResultScene::Initialize()
{
    m_TimeTexId = Texture_Load(L"resource/texture/chip1.png");
    m_ResultTexId = Texture_Load(L"resource/texture/Result.png");
    m_SelectedMenu = RESULT_MENU_RETRY;

    // ラスボス戦で死亡した場合は強化を保持、それ以外はリセット
    if (!GameData_GetDiedInFinalBoss()) {
        PlayerUpgrade_Reset();
    }

    m_finalTime = GameData_GetFinalTime();
    double currentHighScore = GameData_GetHighScore();
    if (currentHighScore == 0.0 || (m_finalTime > 0.0 && m_finalTime < currentHighScore)) {
        GameData_SetHighScore(m_finalTime);
    }

    m_finalshot = ShotCounter_GetScore();
    double currentHighShot = GameData_GetHighShot();
    if (currentHighShot == 0.0 || (m_finalshot > 0.0 && m_finalshot < currentHighShot)) {
        GameData_SetHighShot(m_finalshot);
    }

    m_AccumulatedTime = 0.0;
    m_PrevStickX = 0.0f;

    // BGM読み込み・再生
    m_BgmId = LoadAudio("resource/audio/sea.wav");
    if (m_BgmId != -1) {
        PlayAudio(m_BgmId, true);
    }

    Fade_Start(3.0, false); // フェードイン
}

void ResultScene::Finalize()
{
    // BGM停止・解放
    if (m_BgmId != -1) {
        StopAudio(m_BgmId);
        UnloadAudio(m_BgmId);
        m_BgmId = -1;
    }
}

void ResultScene::Update(double elapsed_time)
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

    if (Fade_GetState() == FADE_STATE_FINISHED_IN)
    {
        // 左右キーで選択切り替え
        bool moveLeft = KeyLogger_IsTrigger(KK_A) || KeyLogger_IsTrigger(KK_LEFT) ||
        Gamepad_IsTrigger(GP_BUTTON_DPAD_LEFT);
        bool moveRight = KeyLogger_IsTrigger(KK_D) || KeyLogger_IsTrigger(KK_RIGHT) ||
        Gamepad_IsTrigger(GP_BUTTON_DPAD_RIGHT);
        
        // スティック入力
        float stickX = Gamepad_GetLeftStickX();
        if (m_PrevStickX <= 0.5f && stickX > 0.5f) moveRight = true;
        if (m_PrevStickX >= -0.5f && stickX < -0.5f) moveLeft = true;
        m_PrevStickX = stickX;
    
        if (moveLeft){
            m_SelectedMenu = RESULT_MENU_RETRY;
        }
        if (moveRight){
            m_SelectedMenu = RESULT_MENU_TITLE;
        }

        // Enterキー、コントローラーAボタン、または左クリックで決定
        if (KeyLogger_IsTrigger(KK_ENTER) || Gamepad_IsTrigger(GP_BUTTON_A) || mouseLeftTrigger){
            Fade_Start(1.0, true);
        }
    }

    if (Fade_GetState() == FADE_STATE_FINISHED_OUT)
    {
        switch (m_SelectedMenu){
        case RESULT_MENU_RETRY:
            Scene_Change(SCENE_GAME);
            break;
        case RESULT_MENU_TITLE:
            Scene_Change(SCENE_TITLE);
            break;
        }
    }
}

void ResultScene::Draw()
{
    Direct3D_SetDepthEnable(false);
    Sprite_Begin();
    
    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());

    // リザルト背景画像を全画面に表示
    Sprite_Draw(m_ResultTexId, 0, 0, screenWidth, screenHeight);

    // 最終タイム
    m_minutes = (int)(m_finalTime / 60);
    m_seconds = (int)m_finalTime % 60;
    m_milliseconds = (int)((m_finalTime - (int)m_finalTime) * 100);

    DrawTime(m_time_x, m_time_y, m_minutes, m_seconds, m_milliseconds);

    // ハイスコア
    double highScore = GameData_GetHighScore();
    m_minutes = (int)(highScore / 60);
    m_seconds = (int)highScore % 60;
    m_milliseconds = (int)((highScore - (int)highScore) * 100);

    DrawTime(m_time_x, m_HighScore_y, m_minutes, m_seconds, m_milliseconds);

    ShotCounter_Draw2(1100, m_time_y, static_cast<int>(m_finalshot));
    ShotCounter_Draw2(1100, m_HighScore_y, static_cast<int>(GameData_GetHighShot()));
}

void ResultScene::DrawTime(float x, float y, int minutes_, int seconds_, int milliseconds_)
{
    float current_x = x;

    drawNumberResult(current_x, y, minutes_ / 10);
    current_x += SCORE_FONT_WIDTH;
    drawNumberResult(current_x, y, minutes_ % 10);
    current_x += SCORE_FONT_WIDTH;

    drawColonResult(current_x, y);
    current_x += SCORE_FONT_WIDTH_SHORT;

    drawNumberResult(current_x, y, seconds_ / 10);
    current_x += SCORE_FONT_WIDTH;
    drawNumberResult(current_x, y, seconds_ % 10);
    current_x += SCORE_FONT_WIDTH;

    drawColonResult(current_x, y);
    current_x += SCORE_FONT_WIDTH_SHORT;

    drawNumberResult(current_x, y, milliseconds_ / 10);
    current_x += SCORE_FONT_WIDTH;
    drawNumberResult(current_x, y, milliseconds_ % 10);
}


void ResultScene::drawNumberResult(float x, float y, int number)
{
    if (number < 0 || number > 9) return;
    Sprite_Draw(m_TimeTexId, x, y, SCORE_FONT_WIDTH, SCORE_FONT_HEIGHT, 32 * number, 0, SCORE_Chip_WIDTH, SCORE_Chip_HEIGHT);
}

void ResultScene::drawColonResult(float x, float y)
{
    Sprite_Draw(m_TimeTexId, x, y, SCORE_FONT_WIDTH_SHORT, SCORE_FONT_HEIGHT, 32 * 10, 0, SCORE_Chip_WIDTH, SCORE_Chip_HEIGHT);
}
