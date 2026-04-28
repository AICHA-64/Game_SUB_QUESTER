// ----------------------------------------------------
// ルート検索 [route_search.h]
// ====================================================
// Created by: Atsushi Yasuda
// Date: 2025-12-15
// Version: 2.0 - 地形対応版
// ----------------------------------------------------
#include "route_search.h"
#include "map.h"
#include "terrain_collision.h"
#include <cmath>
#include <algorithm>
#include <limits>

static constexpr int ROUTE_MAP_WIDTH = 50;
static constexpr int ROUTE_MAP_HEIGHT = 50;
static constexpr int ROUTE_MAP_SIZE = ROUTE_MAP_WIDTH * ROUTE_MAP_HEIGHT;

struct Route
{
	float x{ 0.0f }, y{ 0.0f }, z{ 0.0f }; // Y座標を追加
	bool IsBlocked{ false }; // 通れないかどうかのフラグ
	int route_link[8]{}; // 8方向へのリンクデータ 無い場合は-1
	float slope{ 0.0f }; // 傾斜度（次のマスへの移動コスト計算に使用）
};

static Route g_RouteMap[ROUTE_MAP_SIZE];

struct Point
{
	int x, z;
};

static constexpr Point DIR_OFFSETS[8] = {
	{ 0, -1}, // 上
	{ 1, -1}, // 右上
	{ 1,  0}, // 右
	{ 1,  1}, // 右下
	{ 0,  1}, // 下
	{-1,  1}, // 左下
	{-1,  0}, // 左
	{-1, -1}  // 左上
};

// 移動コスト定数（斜め移動は√2倍）
static constexpr float COST_STRAIGHT = 1.0f;
static constexpr float COST_DIAGONAL = 1.41421356f;

// 地形関連の定数
static constexpr float MAX_CLIMB_HEIGHT = 2.0f;     // 登れる最大高さ
static constexpr float MAX_FALL_HEIGHT = 3.0f;      // 降りられる最大高さ
static constexpr float SLOPE_COST_MULTIPLIER = 2.0f; // 傾斜によるコスト倍率

void RouteSearch_Initialize()
{
	// 地形メッシュを取得
	TerrainMesh* terrain = Map_GetTerrainMesh();
	
	// デバッグ: 地形メッシュの有無を確認
	if (terrain == nullptr) {
#ifdef _DEBUG
		OutputDebugStringA("[RouteSearch] WARNING: TerrainMesh is NULL!\n");
#endif
	} else {
		char buf[256];
		sprintf_s(buf, "[RouteSearch] TerrainMesh found with %d triangles\n", 
			TerrainCollision_GetTriangleCount(terrain));
#ifdef _DEBUG
		OutputDebugStringA(buf);
#endif
	}
	
	
	int validGroundCount = 0;
	int blockedByTerrainCount = 0;
	
	for(int z = 0; z < ROUTE_MAP_HEIGHT; z++) {
		for(int x = 0; x < ROUTE_MAP_WIDTH; x++) {

			int index = x + z * ROUTE_MAP_WIDTH;
			
			float worldX = -25.0f + x + 0.5f; // グリッド中心
			float worldZ = -25.0f + z + 0.5f;
			
			g_RouteMap[index].x = worldX;
			g_RouteMap[index].z = worldZ;
			
			// 地形の高さを取得
			float groundY = 0.0f;
			bool hasGround = false;
			
			if (terrain) {
				DirectX::XMFLOAT3 checkPos = { worldX, 100.0f, worldZ }; // 上空から
				hasGround = TerrainCollision_GetGroundHeight(terrain, checkPos, groundY, nullptr);
			}
			
			if (hasGround) {
				g_RouteMap[index].y = groundY;
				g_RouteMap[index].IsBlocked = false;
				validGroundCount++;
			} else {
				g_RouteMap[index].y = 0.0f;
				g_RouteMap[index].IsBlocked = true; // 地形がない場所は通行不可
				blockedByTerrainCount++;
			}

			// 調査用AABBの作成（地形の高さを基準に設定）
			// yは地面に当たらないスレスレを狙う。大きさはエネミーが通れるサイズ
			AABB route_aabb{ 
				{ worldX - 0.5f, g_RouteMap[index].y, worldZ - 0.5f },
				{ worldX + 0.5f, g_RouteMap[index].y + 1.5f, worldZ + 0.5f } 
			};

			// AABBオブジェクト（配置ブロックなど）との衝突判定
			for(int i = 0; i < Map_GetObjectsCount(); i++) {
				
				if(Collision_IsOverlapAABB(Map_GetObject(i)->Aabb, route_aabb)){
					g_RouteMap[index].IsBlocked = true; // 通れない
					break;
				}
			}
		}
	}
	
	// デバッグ出力
	char debugBuf[512];
	sprintf_s(debugBuf, "[RouteSearch] Initialization complete:\n"
		"  Valid ground cells: %d\n"
		"  Blocked by terrain: %d\n"
		"  Total cells: %d\n",
		validGroundCount, blockedByTerrainCount, ROUTE_MAP_SIZE);
#ifdef _DEBUG
	OutputDebugStringA(debugBuf);
#endif

	// ルートのリンクデータを入れていく
	// リンク先がない場合は-1
	for(int z = 0; z < ROUTE_MAP_HEIGHT; z++) {
		for(int x = 0; x < ROUTE_MAP_WIDTH; x++) {

			int index = x + z * ROUTE_MAP_WIDTH;

			// 8方向を調査
			for(int i = 0; i < 8; i++) {

				int target_x = x + DIR_OFFSETS[i].x;
				int target_z = z + DIR_OFFSETS[i].z;
				int target_index = target_x + target_z * ROUTE_MAP_WIDTH;

				// 範囲外チェックx
				if(target_x < 0 || target_x >= ROUTE_MAP_WIDTH) {
					g_RouteMap[index].route_link[i] = -1; // リンクなし
					continue;
				}

				// 範囲外チェックz
				if(target_z < 0 || target_z >= ROUTE_MAP_HEIGHT) {
					g_RouteMap[index].route_link[i] = -1; // リンクなし
					continue;
				}

				// 障害物チェック
				if(g_RouteMap[target_index].IsBlocked) {
					g_RouteMap[index].route_link[i] = -1; // リンクなし
					continue;
				}

				// 高低差チェック
				float heightDiff = g_RouteMap[target_index].y - g_RouteMap[index].y;
				
				// 登る場合
				if (heightDiff > 0 && heightDiff > MAX_CLIMB_HEIGHT) {
					g_RouteMap[index].route_link[i] = -1; // 急すぎる登り
					continue;
				}
				
				// 降りる場合
				if (heightDiff < 0 && -heightDiff > MAX_FALL_HEIGHT) {
					g_RouteMap[index].route_link[i] = -1; // 急すぎる降り
					continue;
				}

				// 斜め移動の場合、隣接する2マスもチェック（壁の角を通り抜けないように）
				if(i % 2 == 1) { // 斜め方向
					int check1_x = x + DIR_OFFSETS[i].x;
					int check1_z = z;
					int check2_x = x;
					int check2_z = z + DIR_OFFSETS[i].z;
					
					int check1_index = check1_x + check1_z * ROUTE_MAP_WIDTH;
					int check2_index = check2_x + check2_z * ROUTE_MAP_WIDTH;
					
					// 斜めに移動する際、隣接する2マスのどちらかが障害物ならリンクを切る
					if(g_RouteMap[check1_index].IsBlocked || g_RouteMap[check2_index].IsBlocked) {
						g_RouteMap[index].route_link[i] = -1; // リンクなし
						continue;
					}
					
					// 隣接マスの高低差もチェック
					float diff1 = std::abs(g_RouteMap[check1_index].y - g_RouteMap[index].y);
					float diff2 = std::abs(g_RouteMap[check2_index].y - g_RouteMap[index].y);
					if (diff1 > MAX_CLIMB_HEIGHT || diff2 > MAX_CLIMB_HEIGHT) {
						g_RouteMap[index].route_link[i] = -1; // リンクなし
						continue;
					}
				}

				// 傾斜度を計算して保存（移動コスト計算に使用）
				float horizontalDist = (i % 2 == 1) ? COST_DIAGONAL : COST_STRAIGHT;
				g_RouteMap[index].slope = std::abs(heightDiff) / horizontalDist;

				// 問題なければリンクを張る
				g_RouteMap[index].route_link[i] = target_index;
			}
		}
	}
}

void RouteSearch_Finalize()
{
}

struct Node
{
	int route_index{-1};
	float g_cost{ std::numeric_limits<float>::max() }; // 開始地点からの移動コスト
	float h_cost{0}; // ゴール地点までの推定コスト
	int route_parent_index{ -1 }; // どのマスから来たか（親）
	int state{ -1 }; // -1:未調査 0:オープンリスト 1:クローズドリスト
	
	float f_cost() const { return g_cost + h_cost; } // 合計コスト
};

static Node g_MapNode[ROUTE_MAP_SIZE];


// ゴールの座標(gx, gy, gz)と、今の座標(x, y, z)
// 3Dユークリッド距離を使用（Y座標も考慮）
float calculate_heuristic(float x, float y, float z, float gx, float gy, float gz)
{
	float dx = x - gx;
	float dy = y - gy;
	float dz = z - gz;
	return std::sqrtf(dx * dx + dy * dy + dz * dz);
}

// ワールド座標(x,z) -> マップインデックス
int RouteSearch_PositionToIndex(float x, float z)
{
	// グリッドの基準が -25.0f なので +25.0f して整数化
	int mx = static_cast<int>(std::floor(x + 25.0f));
	int mz = static_cast<int>(std::floor(z + 25.0f));

	// 範囲外なら無効インデックスとして -1 を返す
	if (mx < 0 || mx >= ROUTE_MAP_WIDTH || mz < 0 || mz >= ROUTE_MAP_HEIGHT) {
		return -1;
	}

	return mx + mz * ROUTE_MAP_WIDTH;
}

// 最も近い通行可能なマスを探す
int FindNearestWalkableIndex(int index)
{
	if (index < 0 || index >= ROUTE_MAP_SIZE) return -1;
	
	// 既に通行可能ならそのまま返す
	if (!g_RouteMap[index].IsBlocked) return index;
	
	// 周囲8方向を探索
	for (int radius = 1; radius < 10; ++radius) {
		for (int dz = -radius; dz <= radius; ++dz) {
			for (int dx = -radius; dx <= radius; ++dx) {
				if (std::abs(dx) != radius && std::abs(dz) != radius) continue;
				
				int x = (index % ROUTE_MAP_WIDTH) + dx;
				int z = (index / ROUTE_MAP_WIDTH) + dz;
				
				if (x < 0 || x >= ROUTE_MAP_WIDTH || z < 0 || z >= ROUTE_MAP_HEIGHT) continue;
				
				int check_index = x + z * ROUTE_MAP_WIDTH;
				if (!g_RouteMap[check_index].IsBlocked) {
					return check_index;
				}
			}
		}
	}
	
	return -1;
}

RouteData RouteSearch_Search(float x, float z, float target_x, float target_z)
{
	// ノード初期化
	for (Node& node : g_MapNode) {
		node.state = -1; // 未調査
		node.route_index = -1;
		node.g_cost = std::numeric_limits<float>::max();
		node.h_cost = 0;
		node.route_parent_index = -1;
	}

	RouteData ret{};

	int start = RouteSearch_PositionToIndex(x, z);
	int goal = RouteSearch_PositionToIndex(target_x, target_z);

	// 開始／目標がマップ外なら空を返す
	if (start < 0 || goal < 0) {
		return ret;
	}

	// スタート地点が障害物の場合、最も近い通行可能なマスを探す
	if (g_RouteMap[start].IsBlocked) {
		start = FindNearestWalkableIndex(start);
		if (start < 0) return ret;
	}

	// ゴール地点が障害物の場合、最も近い通行可能なマスを探す
	if (g_RouteMap[goal].IsBlocked) {
		goal = FindNearestWalkableIndex(goal);
		if (goal < 0) return ret;
	}

	// スタートノードの初期化（3D距離でヒューリスティック計算）
	g_MapNode[start].route_index = start;
	g_MapNode[start].g_cost = 0.0f;
	g_MapNode[start].h_cost = calculate_heuristic(
		g_RouteMap[start].x, g_RouteMap[start].y, g_RouteMap[start].z,
		g_RouteMap[goal].x, g_RouteMap[goal].y, g_RouteMap[goal].z);
	g_MapNode[start].route_parent_index = -1;
	g_MapNode[start].state = 0; // オープンリストに追加

	int iterations = 0;
	const int MAX_ITERATIONS = 10000; // 無限ループ防止

	// A* メインループ
	while (iterations++ < MAX_ITERATIONS)
	{
		// オープンリストから最小のf_costを持つノードを選択
		int current = -1;
		float minF = std::numeric_limits<float>::max();
		
		for (int i = 0; i < ROUTE_MAP_SIZE; ++i) {
			if (g_MapNode[i].state == 0) { // オープンリスト内
				float f = g_MapNode[i].f_cost();
				if (f < minF) {
					minF = f;
					current = i;
				}
			}
		}

		// オープンリストが空なら経路なし
		if (current == -1) {
			return ret;
		}

		// 現在のノードをクローズドリストに移動
		g_MapNode[current].state = 1;

		// ゴールに到達したら経路を復元
		if (current == goal) {
			int pathNode = current;
			int tempPath[100];
			int pathCount = 0;
			
			// ゴールからスタートへ逆順にたどる
			while (pathNode != -1 && pathCount < 100) {
				tempPath[pathCount++] = g_MapNode[pathNode].route_index;
				pathNode = g_MapNode[pathNode].route_parent_index;
			}
			
			// スタートからゴールへの順序に反転
			for (int i = 0; i < pathCount; ++i) {
				ret.route[i] = tempPath[pathCount - 1 - i];
			}
			ret.count = pathCount;
			
			return ret;
		}

		// 隣接ノードを調査
		for (int i = 0; i < 8; ++i) {
			int neighbor = g_RouteMap[current].route_link[i];
			
			// リンクがないか、クローズドリストにある場合はスキップ
			if (neighbor < 0 || g_MapNode[neighbor].state == 1) {
				continue;
			}

			// 移動コスト計算（斜め移動、高低差、傾斜を考慮）
			float baseCost = (i % 2 == 1) ? COST_DIAGONAL : COST_STRAIGHT;
			
			// 高低差によるコスト加算
			float heightDiff = g_RouteMap[neighbor].y - g_RouteMap[current].y;
			float heightCost = std::abs(heightDiff) * 0.5f; // 高低差1mごとに0.5のコスト
			
			// 傾斜によるコスト加算（急な坂は避けたい）
			float slopeCost = g_RouteMap[current].slope * SLOPE_COST_MULTIPLIER;
			
			float moveCost = baseCost + heightCost + slopeCost;
			float tentative_g = g_MapNode[current].g_cost + moveCost;

			// より良い経路が見つかった、または未調査の場合
			if (g_MapNode[neighbor].state == -1 || tentative_g < g_MapNode[neighbor].g_cost) {
				g_MapNode[neighbor].route_index = neighbor;
				g_MapNode[neighbor].g_cost = tentative_g;
				g_MapNode[neighbor].h_cost = calculate_heuristic(
					g_RouteMap[neighbor].x, g_RouteMap[neighbor].y, g_RouteMap[neighbor].z,
					g_RouteMap[goal].x, g_RouteMap[goal].y, g_RouteMap[goal].z);
				g_MapNode[neighbor].route_parent_index = current;
				
				if (g_MapNode[neighbor].state == -1) {
					g_MapNode[neighbor].state = 0; // オープンリストに追加
				}
			}
		}
	}

	return ret;
}

float RouteSearch_GetXByIndex(int index)
{
	// 範囲チェック
	if (index < 0 || index >= ROUTE_MAP_SIZE) return 0.0f;
	
	return g_RouteMap[index].x;
}

float RouteSearch_GetYByIndex(int index)
{
	// 範囲チェック
	if (index < 0 || index >= ROUTE_MAP_SIZE) return 0.0f;
	
	return g_RouteMap[index].y;
}

float RouteSearch_GetZByIndex(int index)
{
	// 範囲チェック
	if (index < 0 || index >= ROUTE_MAP_SIZE) return 0.0f;

	return g_RouteMap[index].z;
}
