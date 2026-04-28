// ----------------------------------------------------
// マップの制御 [map.cpp]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-11-10
// Version: 1.0
// ----------------------------------------------------
#include "map.h"
using namespace DirectX;
#include "cube.h"
#include "texture.h"
#include "meshfield.h"
#include "light.h"
#include "player_camera.h"
#include "model.h"
#include "map_loader.h"
#include "terrain_collision.h"
#include "player.h"



static MapObject g_MapObjects[]{
	// { FIELD, { 0.0f, 0.0f, 0.0f } , {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},

	{ FIELD01, {-10.0f, -20.0f, -20.0f } , {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}},
};


static int g_CubeTexId = -1; // テクスチャID
static int g_CubefieldTexId = -1; // テクスチャID
static MODEL* g_pFIELD01{};
static MODEL* g_pstage01{};

static MapData g_LoadedMapData;
static std::vector<MapObjectEx> g_DynamicMapObjects;
static bool g_UseLoadedMap = false;

// 地形メッシュ（三角メッシュ当たり判定用）
static TerrainMesh* g_pTerrainMesh = nullptr;

void Map_Initialize()
{

	g_CubeTexId = Texture_Load(L"resource/texture/white.png");
	g_CubefieldTexId = Texture_Load(L"resource/texture/terra-cotta-tile-mid.png");

	//g_pFIELD01 = ModelLoad("resource/model/slime/2020.fbx", 1.0f, false);
	g_pFIELD01 = ModelLoad("resource/model/stage/t4.fbx", 0.005f, false);

	// 地形モデルからTerrainMeshを作成（三角メッシュ当たり判定用）
	XMFLOAT3 terrainPosition = { -10.0f, -20.0f, -20.0f }; // g_MapObjectsのFIELD01と同じ位置
	g_pTerrainMesh = TerrainCollision_CreateFromModel(g_pFIELD01, terrainPosition, 0.005f);
	
	// プレイヤーに地形メッシュを設定
	Player::GetInstance().SetTerrainMesh(g_pTerrainMesh);

	//ですがなんとCubeに限りfor文で設定できる
	for(MapObject& o : g_MapObjects) {
		if (o.KindId == BLOCK01 || o.KindId == BLOCK02) {
			o.Aabb = Cube_GetAABB(o.Position);
		}
		else if (o.KindId == FIELD01)
		{
			//o.Aabb = Model_GetAABB(g_pFIELD01, o.Position);
		}
	}

	// 外部マップファイルを読み込む（没オプション）
	if (MapLoader::LoadFromCSV("resource/map/test_map.csv", g_LoadedMapData, 1.0f)) {
		MapLoader::GenerateMapObjects(g_LoadedMapData, g_DynamicMapObjects, 3.0f);
		g_UseLoadedMap = true;
	}
}

void Map_Finalize()
{
	// 地形メッシュの解放
	if (g_pTerrainMesh) {
		TerrainCollision_Release(g_pTerrainMesh);
		g_pTerrainMesh = nullptr;
	}
	
	ModelRelease(g_pFIELD01);
}

void Map_Draw()
{
	XMMATRIX mtxWorld;

	for (MapObject& o : g_MapObjects) {
		switch (o.KindId)
		{
		case FIELD:
			Light_SetSpecularWorld(PlayerCamera_GetPosition(), 30.0f, { 1.0f, 1.0f, 1.0f, 1.0f });
			// Meshfield_Draw();
			break;
		case BLOCK01:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubefieldTexId, mtxWorld); // どのテクスチャか
			break;
		case BLOCK02:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexId, mtxWorld); // どのテクスチャか
			break;
		case FIELD01:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			ModelDraw(g_pFIELD01, mtxWorld);
			break;
		}
	}
	
}

// シャドウマップ専用描画関数（深度のみ）
void Map_DrawShadow()
{
	XMMATRIX mtxWorld;

	for (MapObject& o : g_MapObjects) {
		switch (o.KindId)
		{
		case BLOCK01:
		case BLOCK02:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_DrawShadow(mtxWorld);
			break;
		case FIELD01:
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			ModelDrawShadow(g_pFIELD01, mtxWorld);
			break;
		}
	}
}

void Map_Draw_DebugSimple()
{
	XMMATRIX mtxWorld;
	for (MapObject& o : g_MapObjects) {
		if (o.KindId == BLOCK01) {
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubefieldTexId, mtxWorld); // back to normal draw now that SRV hazard is fixed
		}
		else if (o.KindId == BLOCK02) {
			mtxWorld = XMMatrixTranslation(o.Position.x, o.Position.y, o.Position.z);
			Cube_Draw(g_CubeTexId, mtxWorld);
		}
	}
}

// int Map_GetObjectsCount()
// {
// 	return sizeof(g_MapObjects)/sizeof(g_MapObjects[0]);
// }
// 
// const MapObject* Map_GetObject(int index)
// {
// 	return &g_MapObjects[index]; // 配列のアドレスを返す
// }

int Map_GetObjectsCount()
{
	if (g_UseLoadedMap) {
		return static_cast<int>(g_DynamicMapObjects.size());
	}
	return sizeof(g_MapObjects) / sizeof(g_MapObjects[0]);
}

const MapObject* Map_GetObject(int index)
{
	if (g_UseLoadedMap && index < static_cast<int>(g_DynamicMapObjects.size())) {
		// MapObjectExからMapObjectへの変換
		static MapObject temp;
		temp.KindId = g_DynamicMapObjects[index].KindId;
		temp.Position = g_DynamicMapObjects[index].Position;
		temp.Aabb = g_DynamicMapObjects[index].Aabb;
		return &temp;
	}
	return &g_MapObjects[index];
}

// 地形メッシュを取得する関数
TerrainMesh* Map_GetTerrainMesh()
{
	return g_pTerrainMesh;
}
