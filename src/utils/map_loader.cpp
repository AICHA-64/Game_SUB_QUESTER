// ----------------------------------------------------
// マップローダー [map_loader.cpp]
// ====================================================
// Created by: Yasuda Atsushi
// Date: 2026-01-06
// Version: 1.0
// ----------------------------------------------------
#include "map_loader.h"
#include <fstream>
#include <sstream>
#include <algorithm>

bool MapLoader::LoadFromCSV(const char* filename, MapData& mapData, float tileSize, const DirectX::XMFLOAT3& origin)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    mapData.tiles.clear();
    mapData.tileSize = tileSize;
    mapData.origin = origin;

    std::vector<std::vector<int>> tempData;
    std::string line;

    // CSVを1行ずつ読み込み
    while (std::getline(file, line)) {
        // 空行やコメント行をスキップ
        if (line.empty() || line[0] == '#') continue;

        std::vector<int> row = ParseCSVLine(line);
        if (!row.empty()) {
            tempData.push_back(row);
        }
    }

    file.close();

    if (tempData.empty()) {
        return false;
    }

    // マップサイズを決定
    mapData.height = static_cast<int>(tempData.size());
    mapData.width = static_cast<int>(tempData[0].size());

    // 全ての行の幅を確認
    for (const auto& row : tempData) {
        mapData.width = std::max(mapData.width, static_cast<int>(row.size()));
    }

    // タイルデータを変換
    mapData.tiles.resize(mapData.width * mapData.height, TileType::EMPTY);

    for (int z = 0; z < mapData.height; ++z) {
        for (int x = 0; x < static_cast<int>(tempData[z].size()); ++x) {
            int index = mapData.GetIndex(x, z);
            if (index >= 0) {
                mapData.tiles[index] = IntToTileType(tempData[z][x]);
            }
        }
    }

    return true;
}

std::vector<AABB> MapLoader::GenerateCollisionAABBs(const MapData& mapData, float wallHeight)
{
    std::vector<AABB> aabbs;

    for (int z = 0; z < mapData.height; ++z) {
        for (int x = 0; x < mapData.width; ++x) {
            TileType type = mapData.GetTileType(x, z);

            // 通行不可のタイルのみ当たり判定を生成
            if (type == TileType::WALL || type == TileType::BLOCK) {
                AABB aabb = mapData.GetTileAABB(x, z, wallHeight);
                aabbs.push_back(aabb);
            }
        }
    }

    return aabbs;
}

bool MapLoader::GenerateMapObjects(const MapData& mapData, std::vector<MapObjectEx>& objects, float wallHeight)
{
    objects.clear();

    for (int z = 0; z < mapData.height; ++z) {
        for (int x = 0; x < mapData.width; ++x) {
            TileType type = mapData.GetTileType(x, z);

            // タイプに応じてオブジェクトを生成
            int kindId = -1;
            DirectX::XMFLOAT3 position = mapData.TileToWorld(x, z);
            position.y = mapData.origin.y + wallHeight * 0.5f; // 高さの中心

            switch (type) {
            case TileType::WALL:
                kindId = 1; // BLOCK01相当
                break;
            case TileType::BLOCK:
                kindId = 2; // BLOCK02相当
                break;
            case TileType::WATER:
                kindId = 3; // 水タイル
                break;
            default:
                continue; // 空きタイルはスキップ
            }

            AABB aabb = mapData.GetTileAABB(x, z, wallHeight);
            objects.emplace_back(kindId, position, aabb);
        }
    }

    return true;
}

std::vector<int> MapLoader::ParseCSVLine(const std::string& line)
{
    std::vector<int> result;
    std::stringstream ss(line);
    std::string cell;

    while (std::getline(ss, cell, ',')) {
        // 空白を除去
        cell.erase(std::remove_if(cell.begin(), cell.end(), ::isspace), cell.end());

        if (!cell.empty()) {
            try {
                result.push_back(std::stoi(cell));
            }
            catch (...) {
                result.push_back(0); // パースエラー時は0
            }
        }
    }

    return result;
}

TileType MapLoader::IntToTileType(int value)
{
    switch (value) {
    case 0: return TileType::EMPTY;
    case 1: return TileType::WALL;
    case 2: return TileType::BLOCK;
    case 3: return TileType::WATER;
    default: return TileType::EMPTY;
    }
}
