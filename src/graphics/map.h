// ----------------------------------------------------
// マップの制御 [map.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-10
// Version: 1.0
// ----------------------------------------------------
#ifndef MAP_H
#define MAP_H


#include <DirectXMath.h>
#include "collision.h"

// 前方宣言
struct TerrainMesh;

void Map_Initialize();
void Map_Finalize();

// void Map_Update(double elapsed_time);
void Map_Draw();

// シャドウマップ専用描画関数（深度のみ）
void Map_DrawShadow();

// Debug helper: draw only block objects (skip complex models) for reflection debugging
void Map_Draw_DebugSimple();

int Map_GetObjectsCount();

enum ObjectKind
{
	FIELD = 0,
	BLOCK01 = 1,
	BLOCK02 = 2,
	FIELD01 = 3,
};

struct MapObject
{
	int KindId; // オブジェクトの種類ID
	DirectX::XMFLOAT3 Position; // 位置
	AABB Aabb; // 当たり判定用AABB
};

const MapObject* Map_GetObject(int index);

// 地形メッシュを取得
TerrainMesh* Map_GetTerrainMesh();

#endif // MAP_H
