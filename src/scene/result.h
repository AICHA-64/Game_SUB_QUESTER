// ----------------------------------------------------
// リザルト画面 [result.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-01-11
// Version: 1.0
// ----------------------------------------------------
#ifndef RESULT_H
#define RESULT_H

#include "scene.h"

enum ResultMenu {
    RESULT_MENU_RETRY,
    RESULT_MENU_TITLE,
    RESULT_MENU_MAX
};

class ResultScene : public IScene {
private:
    int m_BgmId = -1;
    double m_AccumulatedTime = 0.0;
    int m_TimeTexId = -1;
    int m_ResultTexId = -1;
    ResultMenu m_SelectedMenu = RESULT_MENU_RETRY;
    float m_PrevStickX = 0.0f;
    bool m_PrevMouseLeft = false;
    
    double m_finalTime = 0.0;
    double m_finalshot = 0.0;
    float m_time_x = 1450.0f;
    float m_time_y = 580.0f;
    float m_HighScore_y = 330.0f;
    int m_minutes = 0;
    int m_seconds = 0;
    int m_milliseconds = 0;

    void DrawTime(float x, float y, int minutes_, int seconds_, int milliseconds_);
    void drawNumberResult(float x, float y, int number);
    void drawColonResult(float x, float y);

public:

    void Initialize() override;
    void Finalize() override;
    void Update(double elapsed_time) override;
    void Draw() override;
};

#endif // RESULT_H
