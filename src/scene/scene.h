// ----------------------------------------------------
// 逕ｻ髱｢驕ｷ遘ｻ蛻ｶ蠕｡ [scene.h]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2025-07-10
// Version: 1.0
// ----------------------------------------------------
#ifndef SCENE_H
#define SCENE_H

class IScene
{
public:
	virtual ~IScene() {}
	virtual void Initialize() = 0;
	virtual void Finalize() = 0;
	virtual void Update(double elapsed_time) = 0;
	virtual void Draw() = 0;
};

void Scene_Initialize();
void Scene_Finalize();
void Scene_Update(double elapsed_time);
void Scene_Draw();
void Scene_Refresh();


enum Scene
{
	SCENE_TITLE, // 繧ｷ繝ｼ繝ｳ縺悟｢励∴縺溘ｉ縺薙％縺ｫ雜ｳ縺励※縺・￥
	SCENE_GAME,
	SCENE_DOCK,
	SCENE_RESULT,
	SCENE_MAX
};

void Scene_Change(Scene scene);

#endif // SCENE_H
