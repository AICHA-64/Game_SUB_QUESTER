// ----------------------------------------------------
// 画面遷移制御 [scene.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-11
// Version: 1.0
// ----------------------------------------------------

#include "scene.h"
#include "game.h"
#include "title.h"
#include "result.h"
#include "dock.h"
#include "player_upgrade.h"
#include <memory>

// シーン管理データ
struct SceneManager
{
    Scene current = SCENE_TITLE;    // 現在のシーン
    Scene next = SCENE_TITLE;       // 次のシーン
    std::unique_ptr<IScene> instance = nullptr; // シーンの実体
} g_SceneManager;

void Scene_Initialize()
{
    if (!g_SceneManager.instance) {
        switch (g_SceneManager.current) {
        case SCENE_TITLE:
            PlayerUpgrade_Initialize(); // タイトル開始時に強化をリセット
            g_SceneManager.instance = std::make_unique<TitleScene>();
            break;
        case SCENE_GAME:
            g_SceneManager.instance = std::make_unique<GameScene>();
            break;
        case SCENE_DOCK:
            g_SceneManager.instance = std::make_unique<DockScene>();
            break;
        case SCENE_RESULT:
            g_SceneManager.instance = std::make_unique<ResultScene>();
            break;
        }
    }
    
    if (g_SceneManager.instance) {
        g_SceneManager.instance->Initialize();
    }
}

void Scene_Finalize()
{
    if (g_SceneManager.instance) {
        g_SceneManager.instance->Finalize();
    }
}

void Scene_Update(double elapsed_time)
{
    if (g_SceneManager.instance) {
        g_SceneManager.instance->Update(elapsed_time);
    }
}

void Scene_Draw()
{
    if (g_SceneManager.instance) {
        g_SceneManager.instance->Draw();
    }
}

void Scene_Refresh()
{
    if (g_SceneManager.current != g_SceneManager.next) {
        // 現在のシーンの後片付けをする
        Scene_Finalize();

        g_SceneManager.current = g_SceneManager.next; // これをやってから初期化

        // 次のシーンのインスタンスを生成
        switch (g_SceneManager.current) {
        case SCENE_TITLE:
            PlayerUpgrade_Initialize(); // タイトル開始時に強化をリセット
            g_SceneManager.instance = std::make_unique<TitleScene>();
            break;
        case SCENE_GAME:
            g_SceneManager.instance = std::make_unique<GameScene>();
            break;
        case SCENE_DOCK:
            g_SceneManager.instance = std::make_unique<DockScene>();
            break;
        case SCENE_RESULT:
            g_SceneManager.instance = std::make_unique<ResultScene>();
            break;
        }

        // 次のシーンの初期化
        Scene_Initialize();
    }
}

void Scene_Change(Scene scene)
{
    g_SceneManager.next = scene;
}
