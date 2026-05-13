// ----------------------------------------------------
// タイトル制御 [title.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-11
// Version: 1.0
// ----------------------------------------------------

#ifndef TITLE_H
#define TITLE_H

#include "scene.h"

class TitleScene : public IScene {
private:
	int m_TitleTexId = -1;
	double m_AccumulatedTime = 0.0;
	int m_BgmId = -1;
	bool m_PrevMouseLeft = false;

public:
	void Initialize() override;
	void Finalize() override;
	void Update(double elapsed_time) override;
	void Draw() override;
};

#endif // TITLE_H
