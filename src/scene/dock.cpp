// ----------------------------------------------------
// ドック画面（強化選択） [dock.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-10
// Version: 1.1
// ----------------------------------------------------
#include "dock.h"
#include "player_upgrade.h"
#include "mouse.h"
#include "scene.h"
#include "key_logger.h"
#include "gamepad.h"
#include "sprite.h"
#include "texture.h"
#include "direct3d.h"
#include "fade.h"
#include "game.h"
#include "game_window.h"
#include "game_data.h"
#include <cstdio>
#include <Windows.h>

// タイム表示用定数
static constexpr unsigned int TIME_CHIP_WIDTH = 32;
static constexpr unsigned int TIME_CHIP_HEIGHT = 32;
static constexpr int TIME_FONT_WIDTH = 64;
static constexpr int TIME_FONT_HEIGHT = 64;
static constexpr int TIME_FONT_WIDTH_SHORT = 32;

// 強化状態表示用の定数
static constexpr float STATUS_ICON_HEIGHT = 40.0f;      // アイコンサイズ
static constexpr float STATUS_ICON_WIDTH = 100.0f;      // アイコンサイズ
static constexpr float STATUS_PADDING = 10.0f;        // パディング
static constexpr float STATUS_LINE_HEIGHT = 50.0f;   // 各行の高さ

// 強化タイプに対応するテクスチャIDを取得
int DockScene::GetUpgradeTextureId(UpgradeType type)
{
    if (type >= 0 && type < UPGRADE_TYPE_COUNT) {
        return m_UpgradeTexIds[type];
    }
    return -1;
}

void DockScene::Initialize()
{
    // ランダムな強化選択肢を取得
    PlayerUpgrade_GetRandomChoices(&m_Choice1, &m_Choice2);
    
    m_SelectedIndex = 0;
    m_SelectionConfirmed = false;
    
    // テクスチャ読み込み
    m_BackgroundTexId = Texture_Load(L"resource/texture/dock.png");
    m_PanelTexId = Texture_Load(L"resource/texture/Select.png");
    m_TimeChipTexId = Texture_Load(L"resource/texture/chip1.png");
    
    // 強化タイプ別テクスチャ読み込み
    m_UpgradeTexIds[UPGRADE_MOVE_SPEED] = Texture_Load(L"resource/texture/Speed.png");
    m_UpgradeTexIds[UPGRADE_SHOT_INTERVAL] = Texture_Load(L"resource/texture/Bullet.png");
    m_UpgradeTexIds[UPGRADE_MAX_HP] = Texture_Load(L"resource/texture/HP.png");
    m_UpgradeTexIds[UPGRADE_BULLET_SPEED] = Texture_Load(L"resource/texture/Shot.png");
    m_UpgradeTexIds[UPGRADE_DAMAGE] = Texture_Load(L"resource/texture/Power.png");
    
    // 強化状態表示用のパネルテクスチャを読み込み（オプション）
    // m_StatusPanelTexId = Texture_Load(L"resource/texture/status_panel.png");
 
    // フェードイン開始
    Fade_Start(2.0, false);
}

void DockScene::Finalize()
{
}

void DockScene::Update(double /*elapsed_time*/)
{
    // ゲームパッド更新
    // ゲームパッド更新
    Gamepad_Update();
    
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
    
    // フェードアウト完了後にゲームシーンへ
    if (m_SelectionConfirmed && Fade_GetState() == FADE_STATE_FINISHED_OUT) {
        Scene_Change(SCENE_GAME);
        return;
    }
   
    if (m_SelectionConfirmed) {
        return;
    }
    
    // 左右キーまたは左スティックで選択切り替え
    bool moveLeft = KeyLogger_IsTrigger(KK_LEFT) || KeyLogger_IsTrigger(KK_A) ||
        Gamepad_IsTrigger(GP_BUTTON_DPAD_LEFT);
    bool moveRight = KeyLogger_IsTrigger(KK_RIGHT) || KeyLogger_IsTrigger(KK_D) ||
        Gamepad_IsTrigger(GP_BUTTON_DPAD_RIGHT);
    
    // スティック入力（トリガー検出のため前フレーム状態を使う）
    static float prevStickX = 0.0f;
    float stickX = Gamepad_GetLeftStickX();
    if (prevStickX <= 0.5f && stickX > 0.5f) moveRight = true;
    if (prevStickX >= -0.5f && stickX < -0.5f) moveLeft = true;
    prevStickX = stickX;
    
    if (moveLeft && m_SelectedIndex > 0) {
        m_SelectedIndex = 0;
    }
    if (moveRight && m_SelectedIndex < 1) {
        m_SelectedIndex = 1;
    }
    
    // 決定キー
    bool decide = KeyLogger_IsTrigger(KK_ENTER) || KeyLogger_IsTrigger(KK_SPACE) ||
        Gamepad_IsTrigger(GP_BUTTON_A) || mouseLeftTrigger;
    
    if (decide) {
        // 選択した強化を適用
        UpgradeType selected = (m_SelectedIndex == 0) ? m_Choice1 : m_Choice2;
        PlayerUpgrade_Apply(selected);
   
        // ラウンドを進める
        PlayerUpgrade_IncrementRound();
        
        m_SelectionConfirmed = true;
        
        // フェードアウト開始
        Fade_Start(1.0, true);
    }
}

void DockScene::Draw()
{
    Direct3D_SetDepthEnable(false);
    Sprite_Begin();
    
    float screenWidth = static_cast<float>(Direct3D_GetBackBufferWidth());
    float screenHeight = static_cast<float>(Direct3D_GetBackBufferHeight());
    
    // 背景
    Sprite_Draw(m_BackgroundTexId, 0, 0, screenWidth, screenHeight);
    
    // 選択肢テクスチャのサイズ（200×80）に基づいた配置
    float texWidth = 200.0f * 2.5f;   // 少し拡大して表示
    float texHeight = 80.0f * 2.5f;
    float panelY = screenHeight * 0.35f;
    float panel1X = screenWidth * 0.15f;
    float panel2X = screenWidth * 0.55f;
    
    // 選択肢1のカラー
    DirectX::XMFLOAT4 color1 = (m_SelectedIndex == 0) ? 
    DirectX::XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f} :  // 選択中（明るい）
    DirectX::XMFLOAT4{0.5f, 0.5f, 0.5f, 0.7f};   // 非選択（暗い）
    
    // 選択肢2のカラー
    DirectX::XMFLOAT4 color2 = (m_SelectedIndex == 1) ? 
    DirectX::XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f} :
    DirectX::XMFLOAT4{0.5f, 0.5f, 0.5f, 0.7f};
    
    // 選択枠（選択中のみ）
    if (m_SelectedIndex == 0) {
        Sprite_Draw(m_PanelTexId, panel1X - 10, panelY - 10, texWidth + 20, texHeight + 20, {0.3f, 0.7f, 1.0f, 0.8f});
    }
    if (m_SelectedIndex == 1) {
        Sprite_Draw(m_PanelTexId, panel2X - 10, panelY - 10, texWidth + 20, texHeight + 20, {0.3f, 0.7f, 1.0f, 0.8f});
    }
    
    // 強化タイプのテクスチャを描画
    int tex1 = GetUpgradeTextureId(m_Choice1);
    int tex2 = GetUpgradeTextureId(m_Choice2);
    
    // 選択肢1のテクスチャ
    if (tex1 != -1) {
        Sprite_Draw(tex1, panel1X, panelY, texWidth, texHeight, color1);
    } else {
        Sprite_Draw(m_PanelTexId, panel1X, panelY, texWidth, texHeight, color1);
    }
    
    // 選択肢2のテクスチャ
    if (tex2 != -1) {
        Sprite_Draw(tex2, panel2X, panelY, texWidth, texHeight, color2);
    } else {
        Sprite_Draw(m_PanelTexId, panel2X, panelY, texWidth, texHeight, color2);
    }
    
    // タイム表示（選択肢の下に中央配置）
    double finalTime = GameData_GetFinalTime();
    float timeX = screenWidth * 0.5f - 190.0f;  // 中央に配置
    float timeY = panelY + texHeight + 150.0f;   // 選択肢の下
    DrawTime(timeX, timeY, finalTime);
    
    // 強化状態表示（右下）
    float statusX = screenWidth - 300.0f;  // 右端から300px
    float statusY = screenHeight - 400.0f; // 下端から400px
    DrawUpgradeStatus(statusX, statusY);
}

// 数字を描画
void DockScene::DrawNumber(float x, float y, int number)
{
    if (number < 0 || number > 9) return;
    Sprite_Draw(m_TimeChipTexId, x, y, TIME_FONT_WIDTH, TIME_FONT_HEIGHT, 
                TIME_CHIP_WIDTH * number, 0, TIME_CHIP_WIDTH, TIME_CHIP_HEIGHT);
}

// コロンを描画
void DockScene::DrawColon(float x, float y)
{
    Sprite_Draw(m_TimeChipTexId, x, y, TIME_FONT_WIDTH_SHORT, TIME_FONT_HEIGHT, 
    TIME_CHIP_WIDTH * 10, 0, TIME_CHIP_WIDTH, TIME_CHIP_HEIGHT);
}

// タイムを描画 (MM:SS:MS形式)
void DockScene::DrawTime(float x, float y, double time)
{
    int minutes = static_cast<int>(time / 60);
    int seconds = static_cast<int>(time) % 60;
    int milliseconds = static_cast<int>((time - static_cast<int>(time)) * 100);
    
    float currentX = x;
    
    // 分
    DrawNumber(currentX, y, minutes / 10);
    currentX += TIME_FONT_WIDTH;
    DrawNumber(currentX, y, minutes % 10);
    currentX += TIME_FONT_WIDTH;
    
    // コロン
    DrawColon(currentX, y);
    currentX += TIME_FONT_WIDTH_SHORT;
    
    // 秒
    DrawNumber(currentX, y, seconds / 10);
    currentX += TIME_FONT_WIDTH;
    DrawNumber(currentX, y, seconds % 10);
    currentX += TIME_FONT_WIDTH;
    
    // コロン
    DrawColon(currentX, y);
    currentX += TIME_FONT_WIDTH_SHORT;
    
    // ミリ秒
    DrawNumber(currentX, y, milliseconds / 10);
    currentX += TIME_FONT_WIDTH;
    DrawNumber(currentX, y, milliseconds % 10);
}

// 強化状態を描画する関数
void DockScene::DrawUpgradeStatus(float x, float y)
{
    float currentY = y;
        
#if defined(_DEBUG)
    // タイトル表示用の文字列（デバッグ出力として確認用）
    char titleBuf[64];
    sprintf_s(titleBuf, "=== Upgrades ===");
    OutputDebugStringA(titleBuf);
    OutputDebugStringA("\n");
#endif
    
    // 各強化タイプについて表示
    for (int i = 0; i < UPGRADE_TYPE_COUNT; i++) {
        UpgradeType type = static_cast<UpgradeType>(i);
        int level = PlayerUpgrade_GetLevel(type);
        
        // アイコンを描画
        int iconTexId = GetUpgradeTextureId(type);

        if (iconTexId != -1) 
        {
            DirectX::XMFLOAT4 iconColor = {1.0f, 1.0f, 1.0f, 1.0f};

                // レベルが0の場合は暗く表示
                if (level == 0) {
                    iconColor = {0.3f, 0.3f, 0.3f, 0.5f};
                }
                
                Sprite_Draw(iconTexId, x, currentY, STATUS_ICON_WIDTH, STATUS_ICON_HEIGHT, iconColor);
        }
      
        // レベルを表示
        float numberX = x + STATUS_ICON_WIDTH + STATUS_PADDING;
        float numberY = currentY + (STATUS_ICON_HEIGHT - TIME_FONT_HEIGHT) * 0.5f; // 中央揃え
      
        // "x" 記号の代わりにコロンを使用、または数値のみ表示
        if (level >= 10) {
        // 2桁の場合
            DrawNumber(numberX, numberY, level / 10);
            DrawNumber(numberX + TIME_FONT_WIDTH, numberY, level % 10);
        } else {
            // 1桁の場合
            DrawNumber(numberX, numberY, level);
        }
        
#if defined(_DEBUG)
        // デバッグ出力で強化名とレベルを表示
        char buf[128];
        sprintf_s(buf, "%s: Lv.%d", PlayerUpgrade_GetTypeName(type), level);
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
#endif
        
        currentY += STATUS_LINE_HEIGHT;
    }
}
