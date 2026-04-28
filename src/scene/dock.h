// ----------------------------------------------------
// ドック画面（強化選択） [dock.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-02-10
// Version: 1.0
// ----------------------------------------------------
#ifndef DOCK_H
#define DOCK_H

#include "scene.h"
#include "player_upgrade.h" // UpgradeType等が必要なら

class DockScene : public IScene {
private:
    UpgradeType m_Choice1 = UPGRADE_MOVE_SPEED;
    UpgradeType m_Choice2 = UPGRADE_SHOT_INTERVAL;
    int m_SelectedIndex = 0;
    bool m_SelectionConfirmed = false;
    bool m_PrevMouseLeft = false;

    int m_BackgroundTexId = -1;
    int m_PanelTexId = -1;
    int m_TimeChipTexId = -1;
    int m_UpgradeTexIds[UPGRADE_TYPE_COUNT] = { -1, -1, -1, -1, -1 };
    int m_StatusPanelTexId = -1;

    void DrawNumber(float x, float y, int number);
    void DrawColon(float x, float y);
    void DrawTime(float x, float y, double time);
    void DrawUpgradeStatus(float x, float y);
    int GetUpgradeTextureId(UpgradeType type);

public:
    void Initialize() override;
    void Finalize() override;
    void Update(double elapsed_time) override;
    void Draw() override;
};

#endif // DOCK_H
