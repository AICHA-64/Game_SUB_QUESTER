// ----------------------------------------------------
// マップローダー [map_loader.h]
// ====================================================
// Created by:  Atsushi Yasuda
// Date: 2026-01-06
// Version: 1.0
// ----------------------------------------------------
#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <DirectXMath.h>
#include "collision.h"

// マップタイルの種類
enum class TileType
{
    EMPTY = 0,      // 空き（通行可能）
    WALL = 1,       // 壁（通行不可）
    BLOCK = 2,      // ブロック
    WATER = 3,      // 水
    // 必要に応じて追加
};

// マップデータ構造体
struct MapData
{
    int width = 0;              // マップの幅（x軸方向）
    int height = 0;             // マップの高さ（z軸方向）
    float tileSize = 1.0f;      // 1タイルのサイズ
    DirectX::XMFLOAT3 origin = { 0.0f, 0.0f, 0.0f }; // マップの原点座標
    std::vector<TileType> tiles; // タイルデータ（width * height）

    // タイルのインデックスを取得
    int GetIndex(int x, int z) const {
        if (x < 0 || x >= width || z < 0 || z >= height) return -1;
        return x + z * width;
    }

    // 指定座標のタイプを取得
    TileType GetTileType(int x, int z) const {
        int index = GetIndex(x, z);
        if (index < 0) return TileType::EMPTY;
        return tiles[index];
    }

    // ワールド座標をタイル座標に変換
    void WorldToTile(float worldX, float worldZ, int& tileX, int& tileZ) const {
        tileX = static_cast<int>((worldX - origin.x) / tileSize);
        tileZ = static_cast<int>((worldZ - origin.z) / tileSize);
    }

    // タイル座標をワールド座標に変換（タイル中心）
    DirectX::XMFLOAT3 TileToWorld(int tileX, int tileZ) const {
        return DirectX::XMFLOAT3(
            origin.x + (tileX + 0.5f) * tileSize,
            origin.y,
            origin.z + (tileZ + 0.5f) * tileSize
        );
    }

    // 指定タイルのAABBを取得
    AABB GetTileAABB(int tileX, int tileZ, float tileheight = 1.0f) const {
        DirectX::XMFLOAT3 center = TileToWorld(tileX, tileZ);
        float halfSize = tileSize * 0.5f;

        return AABB{
            { center.x - halfSize, origin.y, center.z - halfSize },
            { center.x + halfSize, origin.y + tileheight, center.z + halfSize }
        };
    }
};

// マップローダークラス
class MapLoader
{
public:
    // CSVファイルからマップをロード
    static bool LoadFromCSV(const char* filename, MapData& mapData, float tileSize = 1.0f, const DirectX::XMFLOAT3& origin = { -25.0f, 0.0f, -25.0f });

    // マップデータから当たり判定用のAABBリストを生成
    static std::vector<AABB> GenerateCollisionAABBs(const MapData& mapData, float wallHeight = 3.0f);

    // マップデータからMapObjectリストを生成（map. h互換）
    static bool GenerateMapObjects(const MapData& mapData, std::vector<class MapObjectEx>& objects, float wallHeight = 3.0f);

private:
    // CSVの1行をパース
    static std::vector<int> ParseCSVLine(const std::string& line);

    // 数値をTileTypeに変換
    static TileType IntToTileType(int value);
};

// 拡張MapObject（動的配列用）
class MapObjectEx
{
public:
    int KindId;
    DirectX::XMFLOAT3 Position;
    AABB Aabb;

    MapObjectEx(int kindId, const DirectX::XMFLOAT3& pos, const AABB& aabb)
        : KindId(kindId), Position(pos), Aabb(aabb) {
    }
};

#endif // MAP_LOADER_H
